#include "FFVideoDecoder.h"
#include "PerformanceTimer.h"

#include <Poco/format.h>
#include <Poco/UnicodeConverter.h>


FFVideoDecoder::FFVideoDecoder(Renderer& renderer, AVFormatContext* ic, const int streamNo): FFBaseDecoder(renderer, ic, streamNo),
	_outFrame(NULL), _buffer(NULL), _diFrame(NULL), _diBuffer(NULL), _fx(NULL), _swsCtx(NULL)
{
}

FFVideoDecoder::~FFVideoDecoder() {
	Poco::ScopedLock<Poco::FastMutex> lock(_startLock);
	_worker = NULL;
	_thread.join();

	clearAllFrames();
	if (_fx) {
		_log.information("release effect");
		SAFE_RELEASE(_fx);
	}
}


/**
 * フレームを全てクリアします
 */
void FFVideoDecoder::clearAllFrames() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	int count = 0;
	while (!_usedFrames.empty()) {
		SAFE_DELETE(_usedFrames.front());
		_usedFrames.pop();
		count++;
	}
	while (!_frames.empty()) {
		SAFE_DELETE(_frames.front());
		_frames.pop();
		count++;
	}
	_log.information(Poco::format("release video frames: %d", count));
}

void FFVideoDecoder::start() {
	Poco::ScopedLock<Poco::FastMutex> lock(_startLock);
	AVCodecContext* avctx = _ic->streams[_streamNo]->codec;
//		avctx->thread_count = 4;
//		int res = avcodec_thread_init(avctx, 4);
//		_log.information(Poco::format("thread: %d", res));
	int w = avctx->width;
	int h = avctx->height;
	int flags = SWS_FAST_BILINEAR;
	string type;
	bool changeFormat = false;
	switch (avctx->pix_fmt) {
		case PIX_FMT_YUV420P:
			type = "YUV420P";
			if (createEffect()) {
				changeFormat = false;
			} else {
				// エフェクトが生成できない場合
				changeFormat = true;
				flags = SWS_FAST_BILINEAR;
			}
			break;
		case PIX_FMT_YUYV422:
			type = "YUYV422";
			changeFormat = true;
			flags = SWS_FAST_BILINEAR;
			break;
		case PIX_FMT_RGB24:
			type = "RGB24";
			changeFormat = true;
			break;
		case PIX_FMT_BGR24:
			type = "BGR24";
			changeFormat = true;
			break;
		case PIX_FMT_ARGB:
			type = "RGB32";
			changeFormat = true;
			break;
		case PIX_FMT_YUVJ422P:
			type = "YUVJ422P";
//				changeFormat = true;
//				flags = SWS_FAST_BILINEAR;
			if (createEffect()) {
				changeFormat = false;
			} else {
				// エフェクトが生成できない場合
				changeFormat = true;
				flags = SWS_FAST_BILINEAR;
			}
			break;
		case PIX_FMT_YUVJ444P:
			type = "YUVJ444P";
			changeFormat = true;
			break;
		default:
			type = Poco::format("unknown format(%d)", (int)avctx->pix_fmt);
	}

	string size;
	if (avctx->sample_aspect_ratio.num) {
		_dw = w * avctx->sample_aspect_ratio.num;
		_dh = h * avctx->sample_aspect_ratio.den;
		size = Poco::format("size(%dx%d DAR %d:%d)", w, h, _dw, _dh);
	} else {
		_dw = w;
		_dh = h;
		size = Poco::format("size(%dx%d)", w, h);
	}
	_log.information(Poco::format("video stream: %s format(%s) %d %s", size, type, avctx->ticks_per_frame, string(avctx->hwaccel?"H/W Acceleted":"")));

	if (changeFormat) {
		_outFrame = avcodec_alloc_frame();
		if (_outFrame) {
			int bytes  = avpicture_get_size(PIX_FMT_BGRA, w, h);
			_buffer = (uint8_t*)av_malloc(bytes * sizeof(uint8_t));
			if (_buffer) {
				avpicture_fill((AVPicture*)_outFrame, _buffer, PIX_FMT_BGRA, w, h);
				_swsCtx = sws_getContext(w, h, avctx->pix_fmt, w, h, PIX_FMT_BGRA, flags, NULL, NULL, NULL);
				if (_swsCtx) {
					_log.information(Poco::format("change format by sws: %s -> RGB32", type));
				} else {
					_log.warning(Poco::format("failed not create sws context: %s -> RGB32", type));
					av_free(_outFrame);
					av_free(_buffer);
				}
			} else {
				_log.warning("failed not allocate buffer");
				av_free(_outFrame);
			}
		} else {
			_log.warning("failed not allocate out frame");
		}
	} else {
		_swsCtx = NULL;
	}

	_worker = this;
	_thread.start(*_worker);
}

const float FFVideoDecoder::getDisplayAspectRatio() const {
	return F(_dw) / _dh;
}

const UINT FFVideoDecoder::bufferedFrames() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	return _frames.size() + bufferedPackets();
}

const bool FFVideoDecoder::createEffect() {
	std::wstring wfile;
	Poco::UnicodeConverter::toUTF16(string("conversion_yuv2rgb.fx"), wfile);
	LPD3DXBUFFER errors = NULL;
	HRESULT hr = D3DXCreateEffectFromFile(_renderer.get3DDevice(), wfile.c_str(), 0, 0, D3DXSHADER_DEBUG, 0, &_fx, &errors);
	if (errors) {
		std::vector<char> text(errors->GetBufferSize());
		memcpy(&text[0], errors->GetBufferPointer(), errors->GetBufferSize());
		text.push_back('\0');
		_log.warning(Poco::format("shader compile error: %s", string(&text[0])));
		SAFE_RELEASE(errors);
		return false;
	} else if (FAILED(hr)) {
		_log.warning(Poco::format("failed shader: %s", string("")));
		return false;
	}
	return true;
}

void FFVideoDecoder::run() {
	_log.information("video decoder thread start");
	PerformanceTimer timer;

	AVFrame* frame = avcodec_alloc_frame();
	int gotPicture = 0;
	_readCount = 0;
	_avgTime = 0;

	AVPacketList* packetList = NULL;
	while (_worker) {
		Poco::Thread::sleep(0);
		packetList = popPacket();
		if (!packetList) {
			Poco::Thread::sleep(11);
			continue;
		}
		timer.start();
		AVCodecContext* avctx = _ic->streams[packetList->pkt.stream_index]->codec;
		int bytes = avcodec_decode_video2(avctx, frame, &gotPicture, &packetList->pkt);
		if (gotPicture) {
			const uint8_t* src[4] = {frame->data[0], frame->data[1], frame->data[2], frame->data[3]};
			int pos = avctx->frame_number;
			int w = avctx->width;
			int h = avctx->height;
			bool interlaced = frame->interlaced_frame != 0;
			PixelFormat format = avctx->pix_fmt;
			VideoFrame* vf = popUsedFrame();
			if (PIX_FMT_BGRA == format) {
				if (_swsCtx) {
					if (sws_scale(_swsCtx, src, frame->linesize, 0, h, _outFrame->data, _outFrame->linesize) >= 0) {
						if (!vf || !vf->equals(w, h, D3DFMT_A8R8G8B8)) {
							SAFE_DELETE(vf);
							vf = new VideoFrame(_renderer, w, h, _outFrame->linesize, D3DFMT_A8R8G8B8);
						}
						vf->write(_outFrame);
					} else {
						_log.warning("failed sws_scale");
						SAFE_DELETE(vf);
					}

				} else {
					if (!vf || !vf->equals(w, h, D3DFMT_A8R8G8B8)) {
						SAFE_DELETE(vf);
						vf = new VideoFrame(_renderer, w, h, frame->linesize, D3DFMT_A8R8G8B8);
					}
					vf->write(frame);
				}
			} else {
				if (_swsCtx) {
					if (sws_scale(_swsCtx, src, frame->linesize, 0, h, _outFrame->data, _outFrame->linesize) >= 0) {
						if (!vf || !vf->equals(w, h, D3DFMT_X8R8G8B8)) {
							SAFE_DELETE(vf);
							vf = new VideoFrame(_renderer, w, h, _outFrame->linesize, D3DFMT_X8R8G8B8);
						}
						vf->write(_outFrame);
					} else {
						_log.warning("failed sws_scale");
						SAFE_DELETE(vf);
					}

				} else {
					if (interlaced) {
						// deinterlace
						if (!_diFrame) {
							// deinterlace用のバッファ
							int size = avpicture_get_size(avctx->pix_fmt, frame->linesize[0], h);
							_diBuffer = (uint8_t*)av_malloc(size * sizeof(uint8_t));
							if (_diBuffer) {
								ZeroMemory(_diBuffer, size);
								_diFrame = avcodec_alloc_frame();
								if (_diFrame) {
									avpicture_fill((AVPicture*)_diFrame, _diBuffer, avctx->pix_fmt, frame->linesize[0], h);
									_log.information(Poco::format("created deinterlace buffer: %dx%d %d", frame->linesize[0], h, frame->interlaced_frame));
								} else {
									av_free(_diBuffer);
									_diBuffer = NULL;
								}
							}
						}
						if (_diFrame && avpicture_deinterlace((AVPicture*)_diFrame, (AVPicture*)frame, format, w, h) < 0) {
//								_log.warning("failed deinterlace");
							// movのMJPEGがinterlacedではないのにも関わらず、ここに来てしまう。
							if (!vf || !vf->equals(w, h, D3DFMT_L8)) {
								SAFE_DELETE(vf);
								switch (format) {
								case PIX_FMT_YUV420P:
									vf = new VideoFrame(_renderer, w, h, frame->linesize, h / 2, D3DFMT_L8, _fx);
									break;
								case PIX_FMT_YUVJ422P:
									vf = new VideoFrame(_renderer, w, h, frame->linesize, h, D3DFMT_L8, _fx);
									break;
								}
							}
							vf->write(frame);
						} else {
							//インタレ解除できた
							if (!vf || !vf->equals(w, h, D3DFMT_L8)) {
								SAFE_DELETE(vf);
								switch (format) {
								case PIX_FMT_YUV420P:
									vf = new VideoFrame(_renderer, w, h, frame->linesize, h / 2, D3DFMT_L8, _fx);
									break;
								case PIX_FMT_YUVJ422P:
									vf = new VideoFrame(_renderer, w, h, frame->linesize, h, D3DFMT_L8, _fx);
									break;
								}
							}
							vf->write(_diFrame);
						}
					} else {
						if (!vf || !vf->equals(w, h, D3DFMT_L8)) {
							SAFE_DELETE(vf);
							switch (format) {
							case PIX_FMT_YUV420P:
								vf = new VideoFrame(_renderer, w, h, frame->linesize, h / 2, D3DFMT_L8, _fx);
								break;
							case PIX_FMT_YUVJ422P:
								vf = new VideoFrame(_renderer, w, h, frame->linesize, h, D3DFMT_L8, _fx);
								break;
							}
						}
						vf->write(frame);
					}
				}
			}
			_readTime = timer.getTime();
			_readCount++;
			_avgTime = F(_avgTime * (_readCount - 1) + _readTime) / _readCount;

			if (vf) {
				while (_worker != NULL && _frames.size() >= 50) {
					// キュー空き待ち
					Poco::Thread::sleep(11);
				}
				{
					Poco::ScopedLock<Poco::FastMutex> lock(_lock);
					_frames.push(vf);
//						std::wstring wfile;
//						string file = Poco::format("image%04?i.png", packet.dts);
//						Poco::UnicodeConverter::toUTF16(file, wfile);
//						D3DXSaveTextureToFile(wfile.c_str(), D3DXIFF_PNG, vf->texture[0], NULL);
					// _log.information("queue frame");
				}
			} else {
				_log.warning("failed not created video frame");
			}

		} else {
//				_log.information(Poco::format("video decode not finished: %d", bytes));
		}

		av_free_packet(&packetList->pkt);
		av_freep(&packetList);
	}
	if (_swsCtx) {
		_log.information("release scaler");
		av_free(_outFrame);
		_outFrame = NULL;
		av_free(_buffer);
		_buffer = NULL;
		sws_freeContext(_swsCtx);
		_swsCtx = NULL;
	}

	if (_diFrame) {
		_log.information("release deinterlace buffer");
		av_free(_diFrame);
		_diFrame = NULL;
		av_free(_diBuffer);
		_diBuffer = NULL;
	}

	av_free(frame);
	_worker = NULL;
	_log.information("video decoder thread end");
}

VideoFrame* FFVideoDecoder::popUsedFrame() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (_usedFrames.size() > 10) {
		VideoFrame* vf = _usedFrames.front();
		_usedFrames.pop();
		return vf;
	}
	return NULL;
}

VideoFrame* FFVideoDecoder::popFrame() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (_frames.size() > 0) {
		VideoFrame* vf = _frames.front();
		_frames.pop();
		return vf;
	}
	return NULL;
}

void FFVideoDecoder::pushFrame(VideoFrame* vf) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	_usedFrames.push(vf);
}

VideoFrame* FFVideoDecoder::frontFrame() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (_frames.size() > 0) {
		VideoFrame* vf = _frames.front();
		return vf;
	}
	return NULL;
}

VideoFrame* FFVideoDecoder::viewFrame() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (_frames.size() > 0) {
		VideoFrame* vf = _frames.back();
		return vf;
	}
	return NULL;
}
