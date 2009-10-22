#include "Mover.h"

namespace ui {
	Mover::Mover(string name, UserInterfaceManagerPtr uim, int x, int y, int w, int h, float alpha):
		Component(name, uim, x, y, w, h, alpha), MouseReactionUI(this)
	{
	}

	Mover::~Mover(void) {
		_log.information("delete Mover");
//		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
//		SAFE_RELEASE(_text);
	}

	void Mover::process(const DWORD& frame) {
		if (_enabled && _mouseOver) {
			(_alpha < 0.8f)? _alpha += 0.05f: _alpha = 0.8f;
		} else {
			(_alpha > 0.5f)? _alpha -= 0.05f: _alpha = 0.5f;
		}
	}

	void Mover::draw(const DWORD& frame) {
//		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		DWORD a = ((DWORD)(255 * _alpha)) << 24;
		DWORD c1 = (a | 0xffffff) & _background;
		DWORD c2 = (a | 0x999999) & _background;
		_uim->fillSquare(_x, _y, _w, _h, c1, c1, c2, c2);

		if (_border) {
			c1 = (a | 0xffffff) & _border;
			c2 = (a | 0x999999) & _border;
			_uim->drawSquare(_x, _y, _w, _h, c1, c1, c2, c2);
		}
	}
}
