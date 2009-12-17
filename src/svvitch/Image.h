#pragma once

#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>

#include "Content.h"
#include "MediaItem.h"
#include "PerformanceTimer.h"
#include "Renderer.h"
#include "Workspace.h"

using std::string;
using std::wstring;


class Image: public Content
{
private: 
	Poco::FastMutex _lock;

	int _iw;
	int _ih;
	LPDIRECT3DTEXTURE9 _target;
	int _tw;
	int _th;
	float _dy;

	bool _finished;
	bool _playing;
	PerformanceTimer _playTimer;

public:
	Image(Renderer& renderer):
		Content(renderer), _target(NULL), _finished(true), _playing(false)
	{
		initialize();
	}

	~Image() {
//		_log.information("Image::~Image()");
		initialize();
	}


	void initialize() {
//		_log.information("Image::initialize()");
		close();
	}

	/** ファイルをオープンします */
	bool open(const MediaItemPtr media, const int offset = 0) {
		initialize();

		_iw = 0;
		_ih = 0;
		_dy = 0;
		vector<LPDIRECT3DTEXTURE9> textures;
		bool valid = true;
		for (vector<MediaItemFilePtr>::const_iterator it = media->files().begin(); it != media->files().end(); it++) {
			MediaItemFilePtr mif = *it;
			if (mif->type() == MediaTypeImage) {
				LPDIRECT3DTEXTURE9 texture = _renderer.createTexture(mif->file());
				if (texture) {
					D3DSURFACE_DESC desc;
					HRESULT hr = texture->GetLevelDesc(0, &desc);
					_iw += desc.Width;
					if (_ih < desc.Height) _ih = desc.Height;
					textures.push_back(texture);
					_log.information(Poco::format("opened texture: %s", mif->file()));
				} else {
					_log.warning(Poco::format("failed open: %s", mif->file()));
					valid = false;
				}
			}
		}
		_log.information(Poco::format("tiled texture size: %dx%d", _iw, _ih));
		if (!valid) return false;
//		_tw = 1024;
		_tw = _iw > config().imageSplitWidth?config().imageSplitWidth:_iw;
		int i = (_iw + _tw - 1) / _tw;
		_th = _ih * i;
		_target = _renderer.createRenderTarget(_tw, _th, D3DFMT_X8R8G8B8);
		if (_target) {
			_renderer.colorFill(_target, 0x00000000);
			LPDIRECT3DSURFACE9 dst = NULL;
			HRESULT hr = _target->GetSurfaceLevel(0, &dst);
			if (SUCCEEDED(hr)) {
				LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
				int dx = 0, dy = 0;
				for (vector<LPDIRECT3DTEXTURE9>::iterator it = textures.begin(); it != textures.end(); it++) {
					LPDIRECT3DTEXTURE9 texture = *it;
					D3DSURFACE_DESC desc;
					hr = texture->GetLevelDesc(0, &desc);
					int sx = 0, tw = desc.Width, th = desc.Height;
					while (sx < tw) {
						int w = (tw - sx < _tw - dx)?tw - sx:_tw - dx;

						LPDIRECT3DSURFACE9 src = NULL;
						hr = texture->GetSurfaceLevel(0, &src);
						RECT srcRect = {sx, 0, sx + w, th};
						RECT dstRect = {dx, dy, dx + w, dy + th};
						hr = device->StretchRect(src, &srcRect, dst, &dstRect, D3DTEXF_NONE);
						SAFE_RELEASE(src);
						dx += w;
						sx += w;
						if (dx >= _tw) {
							dx = 0;
							dy += th;
						}
					}
					SAFE_RELEASE(texture);
				}
				SAFE_RELEASE(dst);
			} else {
				_log.warning("failed get surface");
				valid = false;
			}
		} else {
			_log.warning("failed GetSurface()");
			valid = false;
		}

		if (valid) {
			_mediaID = media->id();
			_duration = media->duration() * 60 / 1000;
			_current = 0;
			set("alpha", 1.0f);
			_finished = false;
			return true;
		}

		return false;
	}


	/**
	 * 再生
	 */
	void play() {
		_playing = true;
		_playTimer.start();
	}

	/**
	 * 停止
	 */
	void stop() {
		_playing = false;
	}

	/**
	 * 再生中かどうか
	 */
	const bool playing() const {
		return _playing;
	}

	const bool finished() {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		return _current >= _duration;
	}

	/** ファイルをクローズします */
	void close() {
		stop();
		_mediaID.clear();
		_current = 0;
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			SAFE_RELEASE(_target);
			_tw = 0;
			_th = 0;
		}
	}

	virtual void process(const DWORD& frame) {
		if (_playing) _current++;
	}

	virtual void draw(const DWORD& frame) {
		if (!_mediaID.empty() && _playing) {
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
//			_dy -= 0.5f;
//			if (_dy <= -32) _dy = 32;
			float alpha = getF("alpha");
			DWORD col = ((DWORD)(0xff * alpha) << 24) | 0xffffff;
			int cw = config().splitSize.cx;
			int ch = config().splitSize.cy;
			switch (config().splitType) {
			case 1:
				{
					device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
					RECT scissorRect;
					device->GetScissorRect(&scissorRect);
					device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
					device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
					device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
					device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
					float x = _x;
					float y = _y;
					int mh = config().stageRect.bottom;
					int dh = (mh / ch * cw);
					int ix = 0, sx = 0, sy = 0, dx = (int)x / dh * cw, dxx = (int)fmod(x, cw), dy = ch * ((int)x / cw) % mh;
					int cww = 0;
					int chh = ch;
					while (dx < config().stageRect.right) {
						int cx = dx /cw * dh + dy / ch * cw;
//						if (cx + dxx >= conf->stageRect.right) break;
						if (cx + dxx >= _iw) break;
						RECT rect = {dx, dy, dx + cw, dy + ch};
						device->SetScissorRect(&rect);
						if ((sx + cw - dxx) >= _tw) {
							// srcの右端で折返し
							cww = _tw - sx - dxx;
							_renderer.drawTexture(dx + dxx, y + dy, cww, chh, sx, sy, cww, chh, _target, col, col, col, col);
							sx = 0;
							sy += ch;
							if (sy < _th) {
								// srcを折り返してdstの残りに描画
								if (_th - sy < ch) chh = _th - sy;
								_renderer.drawTexture(dx + dxx + cww, y + dy, cw - cww, chh, sx, sy, cw - cww, chh, _target, col, col, col, col);
								sx += cw - cww;
								ix += cw;
								dxx = cww + cw - cww;
//								if (ix >= _iw) _log.information(Poco::format("image check1: %d,%d %d", dx, dy, dxx));
							} else {
								// dstの途中でsrcが全て終了
								dxx = _iw - ix;
								ix = _iw;
//								if (ix >= _iw) _log.information(Poco::format("image check2: %d,%d %d", dx, dy, dxx));
							}
						} else {
							if (_iw - ix < (cw - dxx)) {
								cww = _iw - ix;
							} else {
								cww = cw - dxx;
							}
							_renderer.drawTexture(dx + dxx, y + dy, cww, chh, sx, sy, cww, chh, _target, col, col, col, col);
							sx += cww;
							ix += cww;
							dxx = cww;
//							if (ix >= _iw) _log.information(Poco::format("image check3: %d,%d %d", dx, dy, dxx));
						}
						if (ix >= _iw) {
							sx = 0;
							sy = 0;
							ix = 0;
							if (dxx < cw) continue;
						}
						dxx = 0;
						dy += ch;
						if (dy >= mh) {
							dx += cw;
							dy = 0;
						}
					}
//					_log.information("image check");
					device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
					device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
					device->SetScissorRect(&scissorRect);
				}
				break;

			case 2:
				{
					device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
					RECT scissorRect;
					device->GetScissorRect(&scissorRect);
					device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
					device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
					device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
					device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
					int cww = cw;
					int chh = ch;
					int sw = _tw / cw;
					if (sw <= 0) {
						sw = 1;
						cww = _tw;
					}
					int sh = _th / ch;
					if (sh <= 0) {
						sh = 1;
						chh = _th;
					}
					for (int sy = 0; sy < sh; sy++) {
						int ox = (sy % 2) * cw * 8 + config().stageRect.left;
						int oy = (sy / 2) * ch * 4 + config().stageRect.top;
						for (int sx = 0; sx < sw; sx++) {
							int dx = (sx / 4) * cw;
							int dy = ch * 3 - (sx % 4) * ch;
							RECT rect = {ox + dx, oy + dy, ox + dx + cw, oy + dy + ch};
							device->SetScissorRect(&rect);
							_renderer.drawTexture(ox + dx + _x, oy + dy + _y, cww, chh, sx * cw, sy * ch, cww, chh, _target, col, col, col, col);
						}
					}
					device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
					device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
					device->SetScissorRect(&scissorRect);
				}
				break;

			default:
				{
					_renderer.drawTexture(0, 0, _target, col, col, col, col);
				}

			}
		} else {
			if (get("prepare") == "true") {
				int sy = getF("itemNo") * 20;
				_renderer.drawTexture(700, 600 + sy, 324, 20, _target, 0, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
			}
		}
	}
};

typedef Image* ImagePtr;