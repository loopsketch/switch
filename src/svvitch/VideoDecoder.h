#pragma once

#include <Poco/Logger.h>
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>
#include <Poco/format.h>
#include <Poco/UnicodeConverter.h>
#include <queue>
#include <string>
#include <mmsystem.h>

#include "BaseDecoder.h"
#include "Renderer.h"
#include "PerformanceTimer.h"

#ifndef _DEBUG
#include <omp.h>
#endif

using std::queue;


class VideoFrame
{
friend class VideoDecoder;
private:
	Poco::Logger& _log;

	Renderer& _renderer;
	int _frameNumber;
	int _ow;
	int _oh;
	int _w[3];
	int _h[3];
	LPDIRECT3DTEXTURE9 texture[3];
	LPD3DXEFFECT _fx;

	const Float toTexelU(const int pixel) {
		return F(pixel) / F(_ow);
	}

	const Float toTexelV(const int pixel) {
		return F(pixel) / F(_oh);
	}


public:
	VideoFrame(Renderer& renderer, const int w, const int h, const int linesize[], const D3DFORMAT format): _log(Poco::Logger::get("")), _renderer(renderer) {
		_ow = abs(linesize[0]) / 4;
		_oh = h;
		_w[0] = w;
		_h[0] = h;
		texture[0] = renderer.createTexture(_ow, _oh, format);
		texture[1] = NULL;
		texture[2] = NULL;
		_fx = NULL;
	}

	VideoFrame(Renderer& renderer, const int w, const int h, const int linesize[], const int h2, const D3DFORMAT format, const LPD3DXEFFECT fx):
		_log(Poco::Logger::get("")), _renderer(renderer), _fx(fx)
	{
		_ow = linesize[0];
		_oh = h;
		_w[0] = w;
		_h[0] = h;
		_w[1] = w / 2;
		_h[1] = h2;
		_w[2] = w / 2;
		_h[2] = h2;
		for (int i = 0; i < 3; i++) {
			texture[i] = renderer.createTexture(linesize[i], _h[i], format);
//			_log.information(Poco::format("texture: <%d>%dx%d", i, linesize[i], _h[i]));
		}
	}

	virtual ~VideoFrame() {
		SAFE_RELEASE(texture[0]);
		SAFE_RELEASE(texture[1]);
		SAFE_RELEASE(texture[2]);
	}

	const int frameNumber() const {
		return _frameNumber;
	}

	const int width() const {
		return _w[0];
	}

	const int height() const {
		return _h[0];
	}

	const bool equals(const int w, const int h, const D3DFORMAT format) {
		if (texture[0]) {
			D3DSURFACE_DESC desc;
			HRESULT hr = texture[0]->GetLevelDesc(0, &desc);
			if (SUCCEEDED(hr) && desc.Format == format && _ow == w && _oh == h) return true;
		}
		return false;
	}

	void write(const AVFrame* frame) {
		if (texture[1]) {
			// プレナー
			D3DLOCKED_RECT lockRect = {0};
			int i;
//#ifndef _DEBUG
//			#pragma omp for private(i)
//#endif
			for (i = 0; i < 3; i++) {
				if (texture[i]) {
					HRESULT hr = texture[i]->LockRect(0, &lockRect, NULL, 0);
					if (SUCCEEDED(hr)) {
						uint8_t* dst8 = (uint8_t*)lockRect.pBits;
						uint8_t* src8 = frame->data[i];
						CopyMemory(dst8, src8, lockRect.Pitch * _h[i]);
						hr = texture[i]->UnlockRect(0);
					} else {
						_log.warning(Poco::format("failed texture[%d] unlock", i));
					}
				}
			}

		} else {
			// パックド
			D3DLOCKED_RECT lockRect = {0};
			HRESULT hr = texture[0]->LockRect(0, &lockRect, NULL, 0);
			if (SUCCEEDED(hr)) {
				uint8_t* dst8 = (uint8_t*)lockRect.pBits;
				uint8_t* src8 = frame->data[0];
				CopyMemory(dst8, src8, lockRect.Pitch * _h[0]);
				hr = texture[0]->UnlockRect(0);
			} else {
				_log.warning("failed lock texture");
			}
		}
		_frameNumber = frame->coded_picture_number;
	}

	void draw(const int x, const int y, int w = -1, int h = -1, int aspectMode = 0, DWORD col = 0xffffffff, int tx = 0, int ty = 0, int tw = -1, int th = -1) {
		if (w < 0) w = _w[0];
		if (h < 0) h = _h[0];
		int dx = 0;
		int dy = 0;
		switch (aspectMode) {
			case 0:
				break;

			case 1:
//				float srcZ =_w[0] / _h[0];

				if (w < _w[0]) {
					float z = F(w) / _w[0];
					float hh = L(_h[0] * z);
					dy = (h - hh) / 2;
					h = hh;
				} else if (h < _h[0]) {
					float z = F(h) / _h[0];
					float ww = L(_w[0] * z);
					dx = (w - ww) / 2;
					w = ww;
				} else {
					if (w > _w[0]) w = _w[0];
					if (h > _h[0]) h = _h[0];
				}
				break;
			default:
				w = _w[0];
				h = _h[0];
		}
		if (tw == -1) tw = _w[0];
		if (th == -1) th = _h[0];

		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		if (texture[1]) {
			// プレナー
			VERTEX dst[] =
				{
					{F(x     + dx - 0.5), F(y     + dy - 0.5), 0.0f, 1.0f, col, toTexelU(tx     ), toTexelV(ty     )},
					{F(x + w + dx - 0.5), F(y     + dy - 0.5), 0.0f, 1.0f, col, toTexelU(tx + tw), toTexelV(ty     )},
					{F(x     + dx - 0.5), F(y + h + dy - 0.5), 0.0f, 1.0f, col, toTexelU(tx     ), toTexelV(ty + th)},
					{F(x + w + dx - 0.5), F(y + h + dy - 0.5), 0.0f, 1.0f, col, toTexelU(tx + tw), toTexelV(ty + th)}
				};
			device->SetTexture(0, texture[0]);
			device->SetTexture(1, texture[1]);
			device->SetTexture(2, texture[2]);
			if (_fx) {
				_fx->SetTechnique("BasicTech");
				_fx->SetTexture("stage0", texture[0]);
				_fx->SetTexture("stage1", texture[1]);
				_fx->SetTexture("stage2", texture[2]);
				_fx->Begin(NULL, 0);
				_fx->BeginPass(0);
			}
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, sizeof(VERTEX));
			if (_fx) {
				_fx->EndPass();
				_fx->End();
			}

			device->SetTexture(0, NULL);
			device->SetTexture(1, NULL);
			device->SetTexture(2, NULL);

		} else {
			// パックド
			VERTEX dst[] =
				{
					{F(x     - 0.5), F(y     - 0.5), 0.0f, 1.0f, col, toTexelU(tx     ), toTexelV(ty     )},
					{F(x + w - 0.5), F(y     - 0.5), 0.0f, 1.0f, col, toTexelU(tx + tw), toTexelV(ty     )},
					{F(x     - 0.5), F(y + h - 0.5), 0.0f, 1.0f, col, toTexelU(tx     ), toTexelV(ty + th)},
					{F(x + w - 0.5), F(y + h - 0.5), 0.0f, 1.0f, col, toTexelU(tx + tw), toTexelV(ty + th)}
				};
			device->SetTexture(0, texture[0]);
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, sizeof(VERTEX));
			device->SetTexture(0, NULL);
		}
//		_renderer.drawFontTextureText(0, 0, 12, 16, 0xffffffff, Poco::format("COLOR: %08lx", col));
	}
};


class VideoDecoder: public BaseDecoder, Poco::Runnable
{
friend class FFMovieContent;
private:
	Poco::FastMutex _startLock;

	Renderer& _renderer;
	AVFormatContext* _ic;
	int _video;

	SwsContext* _swsCtx;
	AVFrame* _outFrame;
	uint8_t* _buffer;

	AVFrame* _diFrame;
	uint8_t* _diBuffer;

	queue<VideoFrame*> _frames;
	queue<VideoFrame*> _usedFrames;

	LPD3DXEFFECT _fx;

	int _dw;
	int _dh;

	VideoDecoder(Renderer& renderer, AVFormatContext* ic, const int video): BaseDecoder(),
		_renderer(renderer), _ic(ic), _video(video), _outFrame(NULL), _buffer(NULL), _diFrame(NULL), _diBuffer(NULL), _fx(NULL), _swsCtx(NULL)
	{
	}

	virtual ~VideoDecoder() {
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
	void clearAllFrames() {
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

	void start() {
		Poco::ScopedLock<Poco::FastMutex> lock(_startLock);
		AVCodecContext* avctx = _ic->streams[_video]->codec;
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

	const float getDisplayAspectRatio() const {
		return F(_dw) / _dh;
	}

	const UINT bufferedFrames() {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		return _frames.size() + bufferedPackets();
	}

	const bool createEffect() {
		std::wstring wfile;
		Poco::UnicodeConverter::toUTF16(string("basic.fx"), wfile);
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

	void run() {
		_log.information("video decoder thread start");
		PerformanceTimer timer;

		AVFrame* frame = avcodec_alloc_frame();
		int gotPicture = 0;
		_readCount = 0;
		_avgTime = 0;

		AVPacketList* packetList = NULL;
		while (_worker) {
			packetList = popPacket();
			if (!packetList) {
				Poco::Thread::sleep(10);
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
					while (_worker != NULL && _frames.size() >= 45) {
						// キュー空き待ち
						//timeBeginPeriod(1);
						Poco::Thread::sleep(10);
						//timeEndPeriod(1);
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
				timeBeginPeriod(1);
				Poco::Thread::sleep(1);
				timeEndPeriod(1);

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

	VideoFrame* popUsedFrame() {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		if (_usedFrames.size() > 10) {
			VideoFrame* vf = _usedFrames.front();
			_usedFrames.pop();
			return vf;
		}
		return NULL;
	}

	VideoFrame* popFrame() {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		if (_frames.size() > 0) {
			VideoFrame* vf = _frames.front();
			_frames.pop();
			return vf;
		}
		return NULL;
	}

	void pushFrame(VideoFrame* vf) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		_usedFrames.push(vf);
	}

	VideoFrame* frontFrame() {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		if (_frames.size() > 0) {
			VideoFrame* vf = _frames.front();
			return vf;
		}
		return NULL;
	}

	VideoFrame* viewFrame() {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		if (_frames.size() > 0) {
			VideoFrame* vf = _frames.back();
			return vf;
		}
		return NULL;
	}
};
