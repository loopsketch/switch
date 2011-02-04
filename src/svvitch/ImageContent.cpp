#include "ImageContent.h"


ImageContent::ImageContent(Renderer& renderer, int splitType, float x, float y, float w, float h):
	Content(renderer, splitType, x, y, w, h), _target(NULL), _finished(true), _playing(false)
{
	initialize();
}

ImageContent::~ImageContent() {
	// _log.information("Image::~Image()");
	initialize();
}


void ImageContent::initialize() {
	// _log.information("Image::initialize()");
	close();
}

bool ImageContent::open(const MediaItemPtr media, const int offset) {
	initialize();

	_iw = 0;
	_ih = 0;
	_dy = 0;
	vector<LPDIRECT3DTEXTURE9> textures;
	bool valid = true;
	D3DFORMAT fomart = D3DFMT_X8R8G8B8; // D3DFMT_A8R8G8B8;
	LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
	int i = 0;
	for (vector<MediaItemFile>::const_iterator it = media->files().begin(); it != media->files().end(); it++) {
		if (i < offset) {
			i++;
			continue;
		}

		MediaItemFile mif = *it;
		if (mif.type() == MediaTypeImage) {
			LPDIRECT3DTEXTURE9 texture = _renderer.createTexture(mif.file());
			if (texture) {
				D3DSURFACE_DESC desc;
				HRESULT hr = texture->GetLevelDesc(0, &desc);
				if (config().splitType != 0 && config().splitSize.cy < desc.Height) {
					// テクスチャ分割
					_ih = config().splitSize.cy;
					LPDIRECT3DSURFACE9 src = NULL;
					hr = texture->GetSurfaceLevel(0, &src);
					for (int y = 0; y < desc.Height; y += _ih) {
						LPDIRECT3DTEXTURE9 t = _renderer.createRenderTarget(desc.Width, _ih, fomart);
						LPDIRECT3DSURFACE9 dst = NULL;
						hr = t->GetSurfaceLevel(0, &dst);
						RECT srcRect = {0, y, desc.Width, y + _ih};
						hr = device->StretchRect(src, &srcRect, dst, NULL, D3DTEXF_NONE);
						if SUCCEEDED(hr) {
							_iw += desc.Width;
							textures.push_back(t);
							// _log.information(Poco::format("texture divid: %02d-%02d", y, (y + _ih - 1)));
						}
						SAFE_RELEASE(dst);
					}
					SAFE_RELEASE(src);
					SAFE_RELEASE(texture);

				} else {
					_iw += desc.Width;
					if (_ih < desc.Height) _ih = desc.Height;
					textures.push_back(texture);
				}
				// _log.information(Poco::format("opened texture: %s", mif.file()));
			} else {
				_log.warning(Poco::format("failed open: %s", mif.file()));
				valid = false;
			}
		} else {
			// MediaTypeMixの場合はファイル単位で処理するので繰り返さない
			if (media->type() == MediaTypeMix) break;
		}
	}
	_log.information(Poco::format("tiled texture size: %dx%d", _iw, _ih));
	if (!valid) return false;
	// _tw = 1024;
	_tw = _iw > config().imageSplitWidth?config().imageSplitWidth:_iw;
	_th = _ih * (_iw + _tw - 1) / _tw;
	_target = _renderer.createRenderTarget(_tw, _th, fomart);
	if (_target) {
		_renderer.colorFill(_target, 0xff000000);
		LPDIRECT3DSURFACE9 dst = NULL;
		HRESULT hr = _target->GetSurfaceLevel(0, &dst);
		if (SUCCEEDED(hr)) {
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


void ImageContent::play() {
	_playing = true;
	_playTimer.start();
}

void ImageContent::stop() {
	_playing = false;
}

const bool ImageContent::playing() const {
	return _playing;
}

const bool ImageContent::finished() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	return _current >= _duration;
}

void ImageContent::close() {
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

void ImageContent::process(const DWORD& frame) {
	if (_playing) {
		_current++;
		if (_duration < _current) _current = _duration;

		int fps = 60;
		unsigned long cu = _current / fps;
		unsigned long re = (_duration - _current) / fps;
		string t1 = Poco::format("%02lu:%02lu:%02lu.%02d", cu / 3600, cu / 60, cu % 60, (_current % fps) / 2);
		string t2 = Poco::format("%02lu:%02lu:%02lu.%02d", re / 3600, re / 60, re % 60, ((_duration - _current) % fps) / 2);
		set("time", Poco::format("%s %s", t1, t2));
		set("time_current", t1);
		set("time_remain", t2);
	}
}

void ImageContent::draw(const DWORD& frame) {
	if (!_mediaID.empty() && _playing) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		// _dy -= 0.5f;
		// if (_dy <= -32) _dy = 32;
		float alpha = getF("alpha", 1.0f);
		DWORD col = ((DWORD)(0xff * alpha) << 24) | 0xffffff;
		const int cw = config().splitSize.cx;
		const int ch = config().splitSize.cy;
		switch (_splitType) {
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
				int ix = 0, sx = 0, sy = 0, dx = (int)x / (cw * config().splitCycles) * cw, dxx = (int)fmod(x, cw), dy = ch * ((int)x / cw) % (config().splitCycles * ch);
				int cww = 0;
				int chh = (ch > _ih)?_ih:ch;
				while (dx < config().mainRect.right) {
					int cx = dx * config().splitCycles + dy / ch * cw; // cx=実際の映像の横位置
					if (cx + dxx >= config().stageRect.right) break;
					RECT rect = {dx, dy, dx + cw, dy + ch};
					device->SetScissorRect(&rect);
					if ((sx + cw - dxx) >= _tw) {
						// srcの右端で折返し
						cww = _tw - sx - dxx;
						_renderer.drawTexture(dx + dxx, y + dy, cww, chh, sx, sy, cww, chh, _target, 0, col, col, col, col);
						sx = 0;
						sy += ch;
						if (sy < _th) {
							// srcを折り返してdstの残りに描画
							if (_th - sy < ch) chh = _th - sy;
							_renderer.drawTexture(dx + dxx + cww, y + dy, cw - cww, chh, sx, sy, cw - cww, chh, _target, 0, col, col, col, col);
							sx += cw - cww;
							ix += cw;
							dxx = cww + cw - cww;
							// if (ix >= _iw) _log.information(Poco::format("image check1: %d,%d %d", dx, dy, dxx));
						} else {
							// dstの途中でsrcが全て終了
							dxx = _iw - ix;
							ix = _iw;
							// if (ix >= _iw) _log.information(Poco::format("image check2: %d,%d %d", dx, dy, dxx));
						}
					} else {
						if (_iw - ix < (cw - dxx)) {
							cww = _iw - ix;
						} else {
							cww = cw - dxx;
						}
						_renderer.drawTexture(dx + dxx, y + dy, cww, chh, sx, sy, cww, chh, _target,0,  col, col, col, col);
						sx += cww;
						ix += cww;
						dxx = cww;
						// if (ix >= _iw) _log.information(Poco::format("image check3: %d,%d %d", dx, dy, dxx));
					}
					if (ix >= _iw) {
						sx = 0;
						sy = 0;
						ix = 0;
						if (dxx < cw) continue;
					}
					dxx = 0;
					dy += ch;
					if (dy >= config().splitCycles * ch) {
						dx += cw;
						dy = 0;
					}
				}
				// _log.information("image check");
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
						_renderer.drawTexture(ox + dx + _x, oy + dy + _y, cww, chh, sx * cw, sy * ch, cww, chh, _target, 0, col, col, col, col);
					}
				}
				device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
				device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
				device->SetScissorRect(&scissorRect);
			}
			break;

		default:
			{
				RECT rect = config().stageRect;
				string aspectMode = get("aspect-mode");
				if (aspectMode == "fit") {
					device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
					device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
					_renderer.drawTexture(L(_x), L(_y), L(_w), L(_h), _target, 0, col, col, col, col);
					device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
					device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);

				} else if (aspectMode == "lefttop") {
					_renderer.drawTexture(L(_x), L(_y), _target, 0, col, col, col, col);

				} else if (aspectMode == "center") {
					int x = _x + (_iw - _w) / 2;
					int y = _y + (_ih - _h) / 2;
					_renderer.drawTexture(x, y, _target, 0, col, col, col, col);

				} else {
					device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
					device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
					if (alpha > 0.0f) {
						DWORD base = ((DWORD)(0xff * alpha) << 24) | 0x000000;
						_renderer.drawTexture(_x, _y, _w, _h, NULL, 0, base, base, base, base);
						float dar = F(_w) / _h;
						if (_h * dar > _w) {
							// 画角よりディスプレイサイズは横長
							long h = _w / dar;
							long dy = (_h - h) / 2;
							_renderer.drawTexture(L(_x), L(_y + dy), L(_w), h, _target, 0, col, col, col, col);
						} else {
							long w = _h * dar;
							long dx = (_w - w) / 2;
							_renderer.drawTexture(L(_x + dx), L(_y), w, L(_h), _target, 0, col, col, col, col);
						}
					}
					device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
					device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
				}
			}

		}
	} else {
		if (get("prepare") == "true") {
			int sy = getF("itemNo") * 20;
			DWORD col = 0xccffffff;
			_renderer.drawTexture(700, 600 + sy, 324, 20, _target, 0, col, col, col, col);
		}
	}
}
