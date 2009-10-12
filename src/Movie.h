#pragma once

#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>
#include "AudioDecoder.h"
#include "VideoDecoder.h"

extern "C" {
#define inline _inline
#include <libavutil/avstring.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
}

#include "Content.h"
#include "FPSCounter.h"
#include "MediaItem.h"
#include "PerformanceTimer.h"
#include "Renderer.h"

using std::string;
using std::wstring;


class Movie: public Content, Poco::Runnable
{
private:
	Poco::FastMutex _lock;
	Poco::FastMutex _frameLock;

	Poco::Thread _thread;
	Poco::Runnable* _worker;

	AVFormatContext* _ic;
	float _rate;
	float _intervals;
	double _lastIntervals;
	int _video;
	int _audio;
	AudioDecoder* _audioDecoder;
	VideoDecoder* _videoDecoder;
	VideoFrame* _vf;
	VideoFrame* _prepareVF;

	bool _finished;
	bool _seeking;
	PerformanceTimer _playTimer;
	FPSCounter _fpsCounter;
	float _avgTime;

public:
	Movie(Renderer& renderer):
		Content(renderer), _ic(NULL), _rate(0), _audioDecoder(NULL), _videoDecoder(NULL), _vf(NULL), _prepareVF(NULL), _finished(true), _seeking(false)
	{
		initialize();
	}

	~Movie() {
		initialize();
	}


	void initialize() {
		close();
		_fpsCounter.start();
		_avgTime = 0;
		if (_vf) {
			Poco::ScopedLock<Poco::FastMutex> lock(_frameLock);
			SAFE_DELETE(_vf);
		}
	}

	/** ファイルをオープンします */
	bool open(const MediaItemPtr media, const int offset = 0) {
		initialize();

		MediaItemFilePtr mif = media->files()[0];
		string file = mif->file();
		string mbfile;
		utf8_sjis(file, mbfile);
		AVInputFormat* format = NULL;
		if (file.find(string("vfwcap")) == 0) {
			_log.information(Poco::format("capture device: %s", file));
			format = av_find_input_format("vfwcap");
			if (format) _log.information(Poco::format("input format: %s", string(format->long_name)));
//			AVOutputFormat* outf = guess_format("vfwcap", mbfile.c_str(), NULL);
//			if (outf) _log.information(Poco::format("output format: [%s]", string(outf->long_name)));

			mbfile = mbfile.substr(6);
			AVFormatParameters ap = {0};
//			ap.prealloced_context = 1;
			ap.width = 640;
			ap.height = 480;
			ap.pix_fmt = PIX_FMT_RGB24; //PIX_FMT_NONE;
			ap.time_base.num = 1;
			ap.time_base.den = 30;
			ap.channel = 0;
//			ap.standard = NULL;
//			ap.video_codec_id = CODEC_ID_NONE;
//			ap.audio_codec_id = CODEC_ID_NONE;

//ap->sample_rate = audio_sample_rate;
//ap->channels = audio_channels;
//ap->time_base.den = frame_rate.num;
//ap->time_base.num = frame_rate.den;
//ap->width = frame_width + frame_padleft + frame_padright;
//ap->height = frame_height + frame_padtop + frame_padbottom;
//ap->pix_fmt = frame_pix_fmt;
// ap->sample_fmt = audio_sample_fmt; //FIXME:not implemented in libavformat
//ap->channel = video_channel;
//ap->standard = video_standard;
//ap->video_codec_id = find_codec_or_die(video_codec_name, CODEC_TYPE_VIDEO, 0);
//ap->audio_codec_id = find_codec_or_die(audio_codec_name, CODEC_TYPE_AUDIO, 0);

			if (av_open_input_file(&_ic, mbfile.c_str(), format, 0, &ap) != 0) {
				_log.warning(Poco::format("failed open capture device: [%s]", mbfile));
				return false;
			}
		} else {
			if (av_open_input_file(&_ic, mbfile.c_str(), NULL, 0, NULL) != 0) {
				_log.warning(Poco::format("failed open file: [%s]", file));
				return false;
			}
		}
//		_log.information(Poco::format("opened: %s", file));

		if (av_find_stream_info(_ic) < 0 || !_ic) {
			_log.warning(Poco::format("failed find stream info: %s", file));
			close();
			return false;
		}
		_log.information(Poco::format("find stream information: %s streams: %u", string(_ic->title), _ic->nb_streams));
		for (int i = 0; i < _ic->nb_streams; i++) {
			AVStream* stream = _ic->streams[i];
			AVCodecContext* avctx = stream->codec;
			if (!avctx) {
				_log.warning(Poco::format("failed stream codec[%d]: %s", i, file));
				continue;
			}
			AVCodec* avcodec = avcodec_find_decoder(avctx->codec_id);
			if (!avcodec) {
				_log.warning(Poco::format("not found decoder[%d]: %s", i, file));
				continue;
			}

			float rate = 0;
			switch (avctx->codec_type) {
				case CODEC_TYPE_VIDEO:
					_duration = stream->duration * stream->time_base.num * stream->r_frame_rate.num / stream->time_base.den / stream->r_frame_rate.den;
					if (_video < 0 && avcodec_open(avctx, avcodec) < 0) {
						_log.warning(Poco::format("failed open codec: %s", file));
					} else {
						// codecがopenできた
						_rate = F(stream->r_frame_rate.num) / stream->r_frame_rate.den;
						_intervals = _renderer.config()->mainRate / _rate;
						_lastIntervals = -1;

						_log.information(Poco::format("open decoder: %s %.3hf %.3hf", string(avcodec->long_name), _rate, _intervals));
						_video = i;
						_videoDecoder = new VideoDecoder(_renderer, _ic, _video);
						_videoDecoder->start();
						_w = avctx->width;
						_h = avctx->height;
					}
					break;

				case CODEC_TYPE_AUDIO:
					if (_audio < 0 && avcodec_open(avctx, avcodec) < 0) {
						_log.warning(Poco::format("failed open codec: %s", file));
					} else {
						// codecがopenできた
						_log.information(Poco::format("open decoder: %s", string(avcodec->long_name)));
						_audio = i;
						_audioDecoder = new AudioDecoder(_renderer, _ic, _audio);
						_audioDecoder->start();
					}
					break;
			}
		}
		_worker = this;
		_thread.start(*_worker);

		_log.information(Poco::format("opened: %s", file));
		_media = media;
		if (_media->duration() > 0) _duration = _media->duration() / 30;
		set("alpha", 1.0f);
		_finished = false;
		return true;
	}

	void run() {
		_log.information("movie thread start");
		int count = 0;
		AVPacket packet;
		while (_worker) {
			if (_videoDecoder && _videoDecoder->bufferedPackets() > 20) {
				Poco::Thread::sleep(10);
				continue;
			}
//			if (_audioDecoder && _audioDecoder->bufferedPackets() > 40) {
//				Poco::Thread::sleep(10);
//				continue;
//			}
			if (_seeking) {
				// シーク中
				Poco::Thread::sleep(10);
				continue;
			}
			if (av_read_frame(_ic, &packet) < 0) {
				// パケット終了 or 異常
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
			if (!_playing) {
				// 停止中はウェイトを増やし負荷を下げる
				Poco::Thread::sleep(50);
			}
			count++;
		}
		_log.information("movie thread end");
	}

	/**
	 * 再生
	 */
	void play() {
		if (_audioDecoder) _audioDecoder->play();
		_current = 0;
		_playing = true;
		_playTimer.start();
	}

	/**
	 * 停止
	 */
	void stop() {
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
	const bool seek(const int64_t timestamp) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		_seeking = true;
		Poco::Thread::sleep(10);
//		if (!_ic || av_seek_frame(_ic, _video, timestamp, 0) < 0) {
		if (!_ic || av_seek_frame(_ic, -1, timestamp, 0) < 0) {
			_log.warning("failed seek frame");
			return false;
		}
		_seeking = false;
		return true;
	}

	const bool finished() {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		if (_playing) {
			if (_finished) {
				if (_videoDecoder) {
					return _videoDecoder->bufferedFrames() == 0;
				}
				return !_media;
			}
		} else {
			return true;
		}
		return false;
	}

	/** ファイルをクローズします */
	void close() {
		_media = NULL;
		if (_ic) {
			stop();
			{
				Poco::ScopedLock<Poco::FastMutex> lock(_frameLock);
				SAFE_DELETE(_vf);
			}
			{
				Poco::ScopedLock<Poco::FastMutex> lock(_lock);
				_worker = NULL;
				_thread.join();
				SAFE_DELETE(_audioDecoder);
				SAFE_DELETE(_videoDecoder);
//				SAFE_DELETE(_vf); // 本来はここが良さそうだがフレーム描画だと落ちるので_videoDecoderが無くなったらdraw()の方でdelete
			}
			for (int i = 0; i < _ic->nb_streams; i++) {
				AVCodecContext* avctx = _ic->streams[i]->codec;
				if (avctx) {
					if (avctx->codec) {
						avcodec_flush_buffers(avctx);
//						avcodec_default_free_buffers(avctx);
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

	virtual void process(const DWORD& frame) {
		if (_media && _videoDecoder) {
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			if (_playing) {
				if (0 == (frame % 2)) {
					VideoFrame* vf = _videoDecoder->popFrame();
					if (vf) {
						if (_vf) _videoDecoder->pushFrame(_vf);
						_vf = vf;
						_current++;
						_fpsCounter.count();
					}
				}
				_avgTime = _videoDecoder->getAvgTime();
				if (_audioDecoder && _videoDecoder->bufferedFrames() == 0) {
					_audioDecoder->stop();
				}
			} else {
				if (get("prepare") == "true") {
					VideoFrame* vf = _videoDecoder->viewFrame();
					if (vf) _prepareVF = vf;
				}
			}
		}
	}

	virtual void draw(const DWORD& frame) {
		if (_media) {
			Poco::ScopedLock<Poco::FastMutex> lock(_frameLock);
			if (_vf && _playing) {
				float alpha = getF("alpha");
				const ConfigurationPtr conf = _renderer.config();
				int cw = conf->splitSize.cx;
				int ch = conf->splitSize.cy;
				DWORD col = ((DWORD)(0xff * alpha) << 24) | 0xffffff;
				switch (conf->splitType) {
				case 1:
					{
						int sx = 0, sy = 0, dx = 0, dy = 0;
						int cww = 0;
						int chh = ch;
						while (dx < 1024) {
							if ((sx + cw) >= _vf->width()) {
								// はみ出る
								cww = _vf->width() - sx;
								_vf->draw(dx, dy, cww, chh, 0, col, sx, sy, cww, chh);
								sx = 0;
								sy += ch;
								if (sy >= _vf->height()) break;
								if (_vf->height() - sy < ch) chh = _vf->height() - sy;
								// クイの分
								_vf->draw(dx + cww, dy, cw - cww, chh, 0, col, sx, sy, cw - cww, chh);
								sx += (cw - cww);
							} else {
								_vf->draw(dx, dy, cw, chh, 0, col, sx, sy, cw, chh);
								sx += cw;
							}
//							_log.information(Poco::format("split dst: %04d,%03d src: %04d,%03d", dx, dy, sx, sy));
							dy += ch;
							if (dy >= 640) {
								dx += cw;
								dy = 0;
							}
						}
					}
					break;

				case 2:
					{
//						device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
//						RECT scissorRect;
//						device->GetScissorRect(&scissorRect);
//						device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
//						device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
//						device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
//						device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
						int sw = _w / cw;
						if (sw <= 0) sw = 1;
						int sh = _h / ch;
						if (sh <= 0) sh = 1;
						for (int sy = 0; sy < sh; sy++) {
							int ox = (sy % 2) * cw * 8 + conf->stageRect.left;
							int oy = (sy / 2) * ch * 4 + conf->stageRect.top;
//							int ox = (sy % 2) * cw * 8;
//							int oy = (sy / 2) * ch * 4;
							for (int sx = 0; sx < sw; sx++) {
								int dx = (sx / 4) * cw;
								int dy = ch * 3 - (sx % 4) * ch;
								_vf->draw(ox + dx, oy + dy, cw, ch, 0, col, sx * cw, sy * ch, cw, ch);
//								_renderer.drawTexture(ox + dx, oy + dy, cw, ch, sx * cw, sy * ch, cw, ch, _target, col, col, col, col);
							}
						}
//						device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
//						device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
//						device->SetScissorRect(&scissorRect);
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
							int w = _renderer.config()->mainRect.right;
							int h = _renderer.config()->mainRect.bottom;
							if (_vf->width() > w || _vf->height() > h) {
								LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
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
					_vf->draw(_x, _y, _w, _h, 0, col);
					break;
				}

			} else if (get("prepare") == "true" && _prepareVF) {
				int sy = getF("itemNo") * 20;
				_prepareVF->draw(700, 600 + sy, 324, 20, 1, 0x99ffffff);
			}
		}
	}

	/**
	 * 再生フレームレート
	 */
	const Uint32 getFPS() {
		return _fpsCounter.getFPS();
	}

	/**
	 * 平均デコード時間
	 */
	const float getAvgTime() const {
		return _avgTime;
	}

	/**
	 * 現在の再生時間
	 */
	const DWORD currentTime() {
		return _playTimer.getTime();
	}

	/**
	 * 残り時間
	 */
	const DWORD timeLeft() {
		long left = (_duration * 1000 / 30) - currentTime();
		if (left < 0) left = 0;
		return left;
	}
};

typedef Movie* MoviePtr;