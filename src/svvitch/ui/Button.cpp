#include "Button.h"

#include <Poco/UnicodeConverter.h>


namespace ui {
	Button::Button(string name, UserInterfaceManagerPtr uim, int x, int y, int w, int h, float alpha):
		Component(name, uim, x, y, w, h, alpha), MouseReactionUI(this), _text(NULL), _dy(0)
	{
	}

	Button::~Button(void) {
		_log.information("delete Button");
//		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		SAFE_RELEASE(_text);
	}

	void Button::setText(const string& text) {
//		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		SAFE_RELEASE(_text);
		if (!text.empty()) {
			_text = _uim->drawText(L"", 18, _color, _color, 1, 0xff000000, 0, 0, text);
		}
	}

	void Button::setText(const wstring& wtext) {
		SAFE_RELEASE(_text);
		if (!wtext.empty()) {
			string text;
			Poco::UnicodeConverter::toUTF8(wtext, text);
			_text = _uim->drawText(L"", 18, _color, _color, 1, 0xff000000, 0, 0, text);
		}
	}

	void Button::setTexture(const LPDIRECT3DTEXTURE9 texture) {
//		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		SAFE_RELEASE(_text);
		_text = texture;
	}

	void Button::process(const DWORD& frame) {
		if (_enabled && _mouseOver) {
			(_alpha < 0.8f)? _alpha += 0.05f: _alpha = 0.8f;
		} else {
			(_alpha > 0.5f)? _alpha -= 0.05f: _alpha = 0.5f;
		}

		if (_lButton) {
			(_dy < 4)?_dy += 1:_dy = 4;
		} else {
			(_dy > 0)?_dy -= 1:_dy = 0;
		}
	}

	void Button::draw(const DWORD& frame) {
//		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		DWORD a = ((DWORD)(255 * _alpha)) << 24;
		DWORD c1 = (a | 0xffffff) & _background;
		DWORD c2 = (a | 0x999999) & _background;
		_uim->fillSquare(_x, _y + _dy, _w, _h, c1, c1, c2, c2);

		if (_text) {
			int x = _x;
			int y = _y;
			D3DSURFACE_DESC desc;
			HRESULT hr = _text->GetLevelDesc(0, &desc);
			if (SUCCEEDED(hr)) {
				int w = desc.Width;
				int h = desc.Height;
				x = _x + (_w - w) / 2;
				y = _y + _dy + (_h - h) / 2;
			}
			float alpha = 0.3f + _alpha;
			if (alpha > F(1)) alpha = F(1);
			c1 = (((DWORD)(255 * alpha)) << 24) | 0xffffff;
			c2 = (((DWORD)(255 * alpha)) << 24) | 0xcccccc;
			_uim->drawTexture(x, y, _text, c1, c1, c2, c2);
		}

		if (_border) {
			c1 = (a | 0xffffff) & _border;
			c2 = (a | 0x999999) & _border;
			_uim->drawSquare(_x, _y + _dy, _w, _h, c1, c1, c2, c2);
		}
	}
}
