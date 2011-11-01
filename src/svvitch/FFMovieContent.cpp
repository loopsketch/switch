#ifdef USE_FFMPEG

#include "FFMovieContent.h"
#include "Utils.h"


FFMovieContent::FFMovieContent(Renderer& renderer, int splitType):
	Content(renderer, splitType), _ic(NULL), _fps(0), _audioDecoder(NULL), _videoDecoder(NULL), _vf(NULL), _prepareVF(NULL),
	_starting(false), _frameOddEven(0), _finished(true), _seeking(false)
{
	initialize();
}

FFMovieContent::~FFMovieContent() {
	Poco::ScopedLock<Poco::FastMutex> lock(_openLock);
	initialize();
}


void FFMovieContent::initialize() {
	close();
	_fpsCounter.start();
	_avgTime = 0;
	if (_vf) {
		Poco::ScopedLock<Poco::FastMutex> lock(_frameLock);
		SAFE_DELETE(_vf);
	}
}

/** ファイルをオープンします */
bool FFMovieContent::open(const MediaItemPtr media, const int offset) {
	Poco::ScopedLock<Poco::FastMutex> lock(_openLock);
	initialize();

	if (media->files().empty() || media->files().size() <= offset) return false;
	MediaItemFile mif = media->files()[offset];
	//for (vector<MediaItemFile>::const_iterator it = media->files().begin(); it != media->files().end(); it++) {
	//	switch (it->type()) {
	//	case MediaTypeMovie:
	//		mif = *it;
	//		break;
	//	}
	//}

	AVInputFormat* format = NULL;
	if (mif.file().find(string("vfwcap")) == 0) {
		_log.information(Poco::format("capture device: %s", mif.file()));
		format = av_find_input_format("vfwcap");
		if (format) _log.information(Poco::format("input format: %s", string(format->long_name)));
		// AVOutputFormat* outf = guess_format("vfwcap", mbfile.c_str(), NULL);
		// if (outf) _log.information(Poco::format("output format: [%s]", string(outf->long_name)));

		AVFormatParameters ap = {0};
		// ap.prealloced_context = 1;
		ap.width = 640;
		ap.height = 480;
		ap.pix_fmt = PIX_FMT_RGB24; //PIX_FMT_NONE;
		ap.time_base.num = 1;
		ap.time_base.den = 30;
		ap.channel = 0;
		//	ap.standard = NULL;
		//	ap.video_codec_id = CODEC_ID_NONE;
		//	ap.audio_codec_id = CODEC_ID_NONE;

		// ap->sample_rate = audio_sample_rate;
		// ap->channels = audio_channels;
		// ap->time_base.den = frame_rate.num;
		// ap->time_base.num = frame_rate.den;
		// ap->width = frame_width + frame_padleft + frame_padright;
		// ap->height = frame_height + frame_padtop + frame_padbottom;
		// ap->pix_fmt = frame_pix_fmt;
		// ap->sample_fmt = audio_sample_fmt; //FIXME:not implemented in libavformat
		// ap->channel = video_channel;
		// ap->standard = video_standard;
		// ap->video_codec_id = find_codec_or_die(video_codec_name, CODEC_TYPE_VIDEO, 0);
		// ap->audio_codec_id = find_codec_or_die(audio_codec_name, CODEC_TYPE_AUDIO, 0);

		if (av_open_input_file(&_ic, mif.file().substr(6).c_str(), format, 0, &ap) != 0) {
			_log.warning(Poco::format("failed open capture device: [%s]", mif.file()));
			return false;
		}
	} else {
		string file = Path(config().dataRoot, Path(mif.file())).absolute().toString();
		string mbfile;
		svvitch::utf8_sjis(file, mbfile);
		if (av_open_input_file(&_ic, mbfile.c_str(), format, 0,NULL) != 0) {
			_log.warning(Poco::format("failed open file: [%s]", file));
			return false;
		}
	}
	// _log.information(Poco::format("opened: %s", file));

	if (av_find_stream_info(_ic) < 0 || !_ic) {
		_log.warning(Poco::format("failed find stream info: %s", mif.file()));
		close();
		return false;
	}
	int64_t start = media->start() / F(1000) * AV_TIME_BASE;
	if (media->start() > 0 && av_seek_frame(_ic, -1, start, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME) != 0) {
		_log.warning(Poco::format("failed set start time: %d", media->start()));
		close();
		return false;
	}

	//_log.information(Poco::format("find stream information: %s streams: %u", string(_ic->title), _ic->nb_streams));
	for (int i = 0; i < _ic->nb_streams; i++) {
		AVStream* stream = _ic->streams[i];
		AVCodecContext* avctx = stream->codec;
		if (!avctx) {
			_log.warning(Poco::format("failed stream codec[%d]: %s", i, mif.file()));
			continue;
		}
		if (avctx->codec_type == AVMEDIA_TYPE_VIDEO) {
			//IDirectXVideoDecoderService* service = NULL;
			//DXVA2CreateVideoService(_renderer.get3DDevice(), IID_PPV_ARGS(&service));
			//UINT resetToken = 0;
		    //IDirect3DDeviceManager9* d3dManager = NULL;
		    //HRESULT hr = DXVA2CreateDirect3DDeviceManager9(&resetToken, &d3dManager);
			//if SUCCEEDED(hr) {
			//	hr = d3dManager->ResetDevice(_renderer.get3DDevice(), resetToken);
			//	if SUCCEEDED(hr) {
			//	} else {
			//		_log.warning("failed ResetDevice()");
			//	}
			//} else {
			//	_log.warning("failed DXVA2CreateDirect3DDeviceManager9()");
			//}
			//SAFE_RELEASE(d3dManager);

			//dxva_context* dxva = (dxva_context*)av_malloc(sizeof(dxva_context));
		//	AVHWAccel* hwaccel = av_hwaccel_next(NULL);
		//	if (hwaccel) {
		//		avctx->hwaccel_context = hwaccel;
		//		avctx->codec_id = hwaccel->id;
		//		_log.information(Poco::format("set hardware accelerator: %s %d", string(hwaccel->name), ((int)hwaccel->pix_fmt)));
		//	}
		}

		AVCodec* avcodec = avcodec_find_decoder(avctx->codec_id);
		if (!avcodec) {
			_log.warning(Poco::format("not found decoder[%d]: %s", i, mif.file()));
			continue;
		}

		float rate = 0;
		switch (avctx->codec_type) {
			case AVMEDIA_TYPE_VIDEO:
				{
					//string name(avcodec->long_name);
					//if (name.find("H.264") != string::npos) {
					//	AVHWAccel* hwaccel = av_hwaccel_next(NULL);
					//	if (hwaccel) {
					//		avctx->hwaccel_context = hwaccel;
					//		_log.information(Poco::format("set hardware accelerator: %s %d", string(hwaccel->name), ((int)hwaccel->pix_fmt)));
					//	}
					//}
				}
				if (_video < 0 && avcodec_open(avctx, avcodec) < 0) {
					_log.warning(Poco::format("failed open codec: %s", mif.file()));
				} else {
					// codecがopenできた
					float rate = F(stream->r_frame_rate.num) / stream->r_frame_rate.den;
					if (rate < 24.5f) {
						_fps = 24;
					} else if (rate < 26.0f) {
						_fps = 25;
					} else if (rate < 31.0f) {
						_fps = 30;
					} else if (rate < 51.0f) {
						_fps = (int)rate;
					} else {
						_fps = 60;
					}
					if (_fps <= 30) {
						_duration = L((stream->duration * F(stream->time_base.num) / stream->time_base.den) * 60);
					} else {
						_duration = stream->duration * stream->time_base.num * stream->r_frame_rate.num / stream->time_base.den / stream->r_frame_rate.den;
					}

					_log.information(Poco::format("open decoder: %s %d(%d/%d) %dkbps", string(avcodec->long_name), _fps, stream->r_frame_rate.num, stream->r_frame_rate.den, avctx->bit_rate / 1024));
					_video = i;
					_videoDecoder = new FFVideoDecoder(_renderer, _ic, _video);
					_videoDecoder->start();
					_w = avctx->width;
					_h = avctx->height;
				}
				break;

			case AVMEDIA_TYPE_AUDIO:
				if (_renderer.getSoundDevice() && config().splitType != 21) {
					if (_audio < 0 && avcodec_open(avctx, avcodec) < 0) {
						_log.warning(Poco::format("failed open codec: %s", mif.file()));
					} else {
						// codecがopenできた
						_log.information(Poco::format("open decoder: %s %dkbps", string(avcodec->long_name), avctx->bit_rate / 1024));
						_audio = i;
						_audioDecoder = new FFAudioDecoder(_renderer, _ic, _audio);
						_audioDecoder->start();
					}
				}
				break;
		}
	}
	if (_audioDecoder && !_audioDecoder->isReady()) return false;
	if (_videoDecoder && !_videoDecoder->isReady()) return false;

	_worker = this;
	_thread.start(*_worker);

	_log.information(Poco::format("opened: %s", mif.file()));
	_mediaID = media->id();
	_current = media->start() * 60 / 1000;
	if (media->duration() > 0) _duration = media->duration() * 60 / 1000;
	set("alpha", 1.0f);
	_starting = false;
	_finished = false;
	return true;
}

void FFMovieContent::run() {
	_log.information("movie thread start");
	long count = 0;
	AVPacket packet;
	while (_worker) {
		Poco::Thread::sleep(0);
		if (!_playing) {
			// 停止中はウェイトを増やし負荷を下げる
			Poco::Thread::sleep(50);
		}
		if (_seeking) {
			// シーク中
			Poco::Thread::sleep(10);
			continue;
		}
		if (_audioDecoder) {
			// audioデコード処理
			_audioDecoder->decode();
			_audioDecoder->writeData();
		}
		if (_videoDecoder) {
			// videoデコード処理
			if (_videoDecoder->bufferedPackets() > 200) {
				Poco::Thread::sleep(10);
				continue;
			}
		}

		if (av_read_frame(_ic, &packet) < 0) {
			// パケット終了 or 異常
			if (!_finished && _audioDecoder) _audioDecoder->finishedPacket();
			_finished = true;
			Poco::Thread::sleep(10);
			continue;
		}

		if (packet.stream_index == _video && _videoDecoder) {
			// 映像
			_videoDecoder->pushPacket(&packet);

		} else if (packet.stream_index == _audio && _audioDecoder) {
			// 音声
			_audioDecoder->pushPacket(&packet);

		} else {
			av_free_packet(&packet);
		}
		count++;
	}
	_log.information(Poco::format("movie thread end %ldpackets", count));
}

/**
 * 再生
 */
void FFMovieContent::play() {
	_playing = true;
	_starting = true;
	_playTimer.start();
}

/**
 * 停止
 */
void FFMovieContent::stop() {
	if (_audioDecoder) _audioDecoder->stop();
	_playing = false;
}

/**
 * Seek to the keyframe at timestamp.
 * 'timestamp' in 'stream_index'.
 * @param stream_index If stream_index is (-1), a default
 * stream is selected, and timestamp is automatically converted
 * from AV_TIME_BASE units to the stream specific time_base.
 * @param timestamp Timestamp in AVStream.time_base units
 *        or, if no stream is specified, in AV_TIME_BASE units.
 * @param flags flags which select direction and seeking mode
 * @return >= 0 on success
 */
const bool FFMovieContent::seek(const int64_t timestamp) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	_seeking = true;
	Poco::Thread::sleep(10);
	// if (!_ic || av_seek_frame(_ic, _video, timestamp, 0) < 0) {
	if (!_ic || av_seek_frame(_ic, -1, timestamp, 0) < 0) {
		_log.warning("failed seek frame");
		return false;
	}
	_seeking = false;
	return true;
}

const bool FFMovieContent::finished() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (_playing) {
		if (_finished) {
			if (_videoDecoder) {
				if (_videoDecoder->bufferedFrames() == 0) {
					if (_audioDecoder && _audioDecoder->playing()) _audioDecoder->stop();
					return true;
				}
				return false;
			}
			return _mediaID.empty();
		}
	} else {
		return true;
	}
	return false;
}

/** ファイルをクローズします */
void FFMovieContent::close() {
	_mediaID.clear();
	if (_ic) {
		stop();
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_frameLock);
			SAFE_DELETE(_vf);
		}
		if (_worker) {
			_worker = NULL;
			_thread.join();
		}
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			SAFE_DELETE(_audioDecoder);
			SAFE_DELETE(_videoDecoder);
			// SAFE_DELETE(_vf); // 本来はここが良さそうだがフレーム描画だと落ちるので_videoDecoderが無くなったらdraw()の方でdelete
		}
		for (int i = 0; i < _ic->nb_streams; i++) {
			AVCodecContext* avctx = _ic->streams[i]->codec;
			if (avctx) {
				if (avctx->codec) {
					avcodec_flush_buffers(avctx);
					// avcodec_default_free_buffers(avctx);
					_log.information(Poco::format("release codec: %s", string(avctx->codec->long_name)));
					avcodec_close(avctx);
				}
			}
		}
		av_close_input_file(_ic);
		_ic = NULL;
	}
	_current = 0;
	_duration = 0;
	_video = -1;
	_audio = -1;
	_finished = true;
}

void FFMovieContent::process(const DWORD& frame) {
	if (!_mediaID.empty() && _videoDecoder) {

		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		int vbufs = 0;
		int abufs = 0;
		if (_playing) {
			if (_starting) {
				_frameOddEven = frame % 2;
				if (_audioDecoder) _audioDecoder->play();
				_starting = false;
			}

			bool popFrame = false;
			switch (config().splitType) {
			case 21:
				break;
			default:
				switch (_fps) {
				case 24:
					{
						// 2-3pulldown
						int p = (frame % 5);
						popFrame = p < 4 && _frameOddEven == (p % 2);
					}
					break;
				case 25:
					{
						int p = (frame % 12);
						popFrame = p == 0 || p == 2 || p == 5 || p == 7 || p == 9;
					}
					break;
				case 30:
					popFrame = _frameOddEven == (frame % 2);
					break;
				case 60:
					popFrame = true;
					break;
				default:
					popFrame = (frame % (60 / (60 - _fps))) != 0;
					break;
				}
				if (popFrame) {
					VideoFrame* vf = _videoDecoder->popFrame();
					if (vf) {
						if (_vf) _videoDecoder->pushUsedFrame(_vf);
						_vf = vf;
						_fpsCounter.count();
					}
				}
				_current++;
			}
			if (_videoDecoder) vbufs = _videoDecoder->bufferedFrames();
			if (_audioDecoder) abufs = _audioDecoder->bufferedFrames();
			_avgTime = _videoDecoder->getAvgTime();

		} else {
			_fpsCounter.start();
			if (get("prepare") == "true") {
				VideoFrame* vf = _videoDecoder->viewFrame();
				if (vf) _prepareVF = vf;
			}
		}
		unsigned long cu = _current / 60;
		int remain = _duration - _current;
		if (remain < 0) remain = 0;
		unsigned long re = remain / 60;
		string t1 = Poco::format("%02lu:%02lu:%02lu.%02d", cu / 3600, cu / 60, cu % 60, (_current / 2) % 30);
		string t2 = Poco::format("%02lu:%02lu:%02lu.%02d", re / 3600, re / 60, re % 60, (remain / 2) % 30);
		set("time", Poco::format("%s %s", t1, t2));
		set("time_current", t1);
		set("time_remain", t2);
		//set("time_fps", Poco::format("%d(%0.2hf)", fps, _rate));

		set("status", Poco::format("%dp(%02lufps-%03.2hfms) %02d:%02d", _fps, _fpsCounter.getFPS(), _avgTime, vbufs, abufs));
	}
}

void FFMovieContent::draw(const DWORD& frame) {
	if (!_mediaID.empty()) {
		Poco::ScopedLock<Poco::FastMutex> lock(_frameLock);
		if (_vf && _playing) {
			LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
			const float alpha = getF("alpha");
			const int cw = config().splitSize.cx;
			const int ch = config().splitSize.cy;
			DWORD col = ((DWORD)(0xff * alpha) << 24) | 0xffffff;
			switch (_splitType) {
			case 1:
				{
					int sx = 0, sy = 0, dx = 0, dy = 0;
					int cww = 0;
					int chh = ch;
					while (dx < config().mainRect.right) {
						int ix = dx * config().splitCycles + dy / ch * cw;
						if (ix >= config().stageRect.right) break;
						if ((sx + cw) >= _vf->width()) {
							// はみ出る
							cww = _vf->width() - sx;
							if (ix + cww > config().stageRect.right) {
								cww = config().stageRect.right - ix;
							}
							_vf->draw(dx, dy, cww, chh, 0, col, sx, sy, cww, chh);
							sx = 0;
							sy += ch;
							if (ix + cww >= config().stageRect.right) break;
							if (sy >= _vf->height()) {
								sy = 0;
								chh = ch;
								_vf->draw(dx + cww, dy, cw - cww, chh, 0, col, sx, sy, cw - cww, chh);
								sx += (cw - cww);
							} else {
								if (_vf->height() - sy < ch) chh = _vf->height() - sy;
								// クイの分
								_vf->draw(dx + cww, dy, cw - cww, chh, 0, col, sx, sy, cw - cww, chh);
								sx += (cw - cww);
							}
						} else {
							cww = config().stageRect.right - ix;
							if (cw > cww) {
								_vf->draw(dx, dy, cww, chh, 0, col, sx, sy, cww, chh);
								break;
							} else {
								_vf->draw(dx, dy, cw, chh, 0, col, sx, sy, cw, chh);
								sx += cw;
							}
						}
						// _log.information(Poco::format("split dst: %04d,%03d src: %04d,%03d", dx, dy, sx, sy));
						dy += ch;
						if (dy >= config().stageRect.bottom * config().splitCycles) {
							dx += cw;
							dy = 0;
						}
					}
				}
				break;

			case 2:
				{
//					device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
//					RECT scissorRect;
//					device->GetScissorRect(&scissorRect);
//					device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
//					device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
//					device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
//					device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
					int sw = L(_w / cw);
					if (sw <= 0) sw = 1;
					int sh = L(_h / ch);
					if (sh <= 0) sh = 1;
					for (int sy = 0; sy < sh; sy++) {
						int ox = (sy % 2) * cw * 8 + config().stageRect.left;
						int oy = (sy / 2) * ch * config().splitCycles + config().stageRect.top;
						// int ox = (sy % 2) * cw * 8;
						// int oy = (sy / 2) * ch * 4;
						for (int sx = 0; sx < sw; sx++) {
							int dx = (sx / config().splitCycles) * cw;
							int dy = ch * (config().splitCycles - 1) - (sx % config().splitCycles) * ch;
							_vf->draw(ox + dx, oy + dy, cw, ch, 0, col, sx * cw, sy * ch, cw, ch);
//							_renderer.drawTexture(ox + dx, oy + dy, cw, ch, sx * cw, sy * ch, cw, ch, _target, col, col, col, col);
						}
					}
//					device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
//					device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
//					device->SetScissorRect(&scissorRect);
				}
				break;

			case 11:
				{
					if (_vf->height() == 120) {
						_vf->draw(0, 360, 320, 120, 0, col, 2880, 0, 320, 120);
						_vf->draw(0, 240, 960, 120, 0, col, 1920, 0, 960, 120);
						_vf->draw(0, 120, 960, 120, 0, col,  960, 0, 960, 120);
						_vf->draw(0,   0, 960, 120, 0, col,    0, 0, 960, 120);
					} else {
						int w = config().mainRect.right;
						int h = config().mainRect.bottom;
						if (_vf->width() > w || _vf->height() > h) {
							device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
							device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
							_vf->draw( 0, 0, w, h, 1, col);
							device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
							device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
						} else {
//							_vf->draw(0, 0);
							_vf->draw(0, 0, -1, -1, 0, col);
						}
					}
				}
				break;

			default:
				{
					RECT rect = config().stageRect;
					string aspectMode = get("aspect-mode");
					if (aspectMode == "fit") {
						device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
						device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
						_vf->draw(L(_x), L(_y), L(_w), L(_h), 0, col);
						device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
						device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);

					} else {
						device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
						device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
						if (alpha > 0.0f) {
							DWORD base = ((DWORD)(0xff * alpha) << 24) | 0x000000;
							_renderer.drawTexture(_x, _y, _w, _h, NULL, 0, base, base, base, base);
							float dar = _videoDecoder->getDisplayAspectRatio();
							if (_h * dar > _w) {
								// 画角よりディスプレイサイズは横長
								long h = _w / dar;
								long dy = (_h - h) / 2;
								_vf->draw(L(_x), L(_y + dy), L(_w), h, 0, col);
							} else {
								long w = _h * dar;
								long dx = (_w - w) / 2;
								_vf->draw(L(_x + dx), L(_y), w, L(_h), 0, col);
							}
						}

						device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
						device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
					}
				}
				break;
			}

		} else if (_playing && config().splitType == 21) {
			LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
			float alpha = getF("alpha");
			int cw = config().splitSize.cx;
			int ch = config().splitSize.cy;
			DWORD col = ((DWORD)(0xff * alpha) << 24) | 0xffffff;

			int remain = _duration - _current;
			if (_videoDecoder->bufferedFrames() >= ((remain > 8)?8:remain)) {
				for (int y = 0; y < 2; y++) {
					for (int x = 0; x < 4 && _current <= _duration; x++) {
						VideoFrame* vf = _videoDecoder->popFrame();
						if (vf) {
							int w = vf->width();
							int h = vf->height();
							vf->draw(L(x * w), L(y * h), L(w), L(h), 0, 0xffffffff);
							_videoDecoder->pushUsedFrame(vf);
							_fpsCounter.count();
							_current++;
						}
					}
				}
				_fpsCounter.count();
			}

		} else if (get("prepare") == "true" && _prepareVF) {
			int sy = L(getF("itemNo") * 20);
			_prepareVF->draw(700, 600 + sy, 324, 20, 1, 0x99ffffff);
		}
	}
}

/**
 * 再生フレームレート
 */
const Uint32 FFMovieContent::getFPS() {
	return _fpsCounter.getFPS();
}

/**
 * 平均デコード時間
 */
const float FFMovieContent::getAvgTime() const {
	return _avgTime;
}

/**
 * 現在の再生時間
 */
const DWORD FFMovieContent::currentTime() {
	return _playTimer.getTime();
}

/**
 * 残り時間
 */
const DWORD FFMovieContent::timeLeft() {
	long left = (_duration * 1000 / 30) - currentTime();
	if (left < 0) left = 0;
	return left;
}

#endif
