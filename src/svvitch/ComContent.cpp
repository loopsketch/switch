#include "ComContent.h"


ComContent::ComContent(Renderer& renderer, int splitType, float x, float y, float w, float h):
	Content(renderer, splitType, x, y, w, h), _ole(NULL), _controlSite(NULL), _texture(NULL), _surface(NULL),
	_phase(-1)
{
}

ComContent::~ComContent() {
}

void ComContent::invalidateRect(int x, int y, int w, int h) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	_invalidateRects.push(Rect(x, y, w, h));
}

bool ComContent::hasInvalidateRect() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	return !_invalidateRects.empty();
}

Rect ComContent::popInvalidateRect() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	Rect rect = _invalidateRects.front();
	_invalidateRects.pop();
	return rect;
}

bool ComContent::open(const MediaItemPtr media, const int offset) {
	_controlSite = new ControlSite(this);
	_controlSite->AddRef();
	_texture = _renderer.createTexture(_w, _h, D3DFMT_A8R8G8B8);
	if (_texture) {
		_log.information(Poco::format("flash texture: %.0hfx%.0hf", _w, _h));
		_texture->GetSurfaceLevel(0, &_surface);
	}

	set("alpha", 1.0f);
	_duration = media->duration() * 60 / 1000;
	_current = 0;
	_mediaID = media->id();
	return true;
}

void ComContent::play() {
	_playing = true;
	_playTimer.start();
}

void ComContent::stop() {
	_playing = false;
	if (_phase >= 0) {
		_thread.join();
		releaseComComponents();
	}
}

bool ComContent::useFastStop() {
	return true;
}

const bool ComContent::playing() const {
	return _playing;
}

const bool ComContent::finished() {
	switch (_phase) {
	case 0:
	case 1:
	case 2:
		if (_duration > 0) {
			return _playTimer.getTime() >= 1000 * _duration / 60;
		}
		break;
	case 3:
		return true;
	}
	return false;
}

void ComContent::close() {
	stop();
	_mediaID.clear();
	SAFE_RELEASE(_surface);
	SAFE_RELEASE(_texture);
	SAFE_RELEASE(_controlSite);
}

void ComContent::process(const DWORD& frame) {
	switch (_phase) {
	case 0: // 初期化フェーズ
		if (!_mediaID.empty()) createComComponents();
		break;
	case 1: // 初期化済
		if (_playing && !_mediaID.empty()) {
			_worker = this;
			_thread.start(*_worker);
			_phase = 2;
		}
		break;
	case 2: // 再生中
		_current = _playTimer.getTime() * 60 / 1000;
		break;
	case 3: // リリース済
		break;
	}
	unsigned long cu = _playTimer.getTime() / 1000;
	unsigned long re = _duration / 60 - cu;
	string t1 = Poco::format("%02lu:%02lu:%02lu.%02d", cu / 3600, cu / 60, cu % 60, 0);
	string t2 = Poco::format("%02lu:%02lu:%02lu.%02d", re / 3600, re / 60, re % 60, 0);
	set("time", Poco::format("%s %s", t1, t2));
	set("time_current", t1);
	if (_duration > 0) {
		set("time_remain", t2);
	} else {
		set("time_remain", "--:--:--.--");
	}
	set("status", Poco::format("%03.2hfms", _avgTime));
}


void ComContent::draw(const DWORD& frame) {
	switch (_phase) {
	case 2:
	case 3:
		if (_texture) {
			float alpha = getF("alpha");
			DWORD col = ((DWORD)(0xff * alpha) << 24) | 0xffffff;
			int cw = config().splitSize.cx;
			int ch = config().splitSize.cy;
			LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
			switch (config().splitType) {
			case 1:
				{
				}
				break;
			case 2:
				{
					// device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
					// RECT scissorRect;
					// device->GetScissorRect(&scissorRect);
					device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
					device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
					device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
					device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
					int sw = _w / cw;
					int sh = _h / ch;
					for (int sy = 0; sy < sh; sy++) {
						for (int sx = 0; sx < sw; sx++) {
							int dx = (sx / config().splitCycles) * cw;
							int dy = (config().splitCycles - (sx % config().splitCycles) - 1) * ch;
							// RECT rect = {dx, dy, dx + cww, dy + chh};
							// device->SetScissorRect(&rect);
							if (_background) {
								_renderer.drawTexture(dx + _x, dy + _y, cw, ch, sx * cw, sy * ch, cw, ch, NULL, 0, _background, _background, _background, _background);
							}
							_renderer.drawTexture(dx + _x, dy + _y, cw, ch, sx * cw, sy * ch, cw, ch, _texture, 0, col, col, col, col);
						}
					}
					device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
					device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
					// device->SetScissorRect(&scissorRect);
				}
				break;
			default:
				if (_background) {
					_renderer.drawTexture(_x, _y, _w, _h, NULL, 0, _background, _background, _background, _background);
				}
				_renderer.drawTexture(_x, _y, _texture, 0, col, col, col, col);
			}
		}
		break;
	}
}
