#include "Mover.h"

namespace ui {
	Mover::Mover(string name, UserInterfaceManagerPtr uim, int x, int y, int w, int h, float alpha):
		Component(name, uim, x, y, w, h, alpha), MouseReactionUI(this), _dx(0), _dy(0)
	{
	}

	Mover::~Mover(void) {
		_log.information("delete Mover");
//		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
//		SAFE_RELEASE(_text);
	}

	void Mover::setMovingBounds(int x, int y, int w, int h) {
		_mx = x;
		_my = y;
		_mw = w;
		_mh = h;
	}

	int Mover::getX() {
		return _x + _dx;
	}

	int Mover::getY() {
		return _y + _dy;
	}

	void Mover::process(const DWORD& frame) {
		if (_enabled && _mouseOver) {
			(_alpha < 0.8f)? _alpha += 0.05f: _alpha = 0.8f;
		} else {
			(_alpha > 0.5f)? _alpha -= 0.05f: _alpha = 0.5f;
		}

		if (_lButtonDrag) {
			if (_mw > 0) {
				_dx = _mouseX - _dragX;
				int xl = _mx - _x;
				int xh = _mx + _mw - _x - _w;
				if (_dx < xl) _dx = xl; else if (_dx > xh) _dx = xh;
			}
			if (_mh > 0) {
				_dy = _mouseY - _dragY;
				int yl = _my - _y;
				int yh = _my + _mh - _y - _h;
				if (_dy < yl) _dy = yl; else if (_dy > yh) _dy = yh;
			}
		} else {
			if (_dx != 0) _x = _x + _dx;
			if (_dy != 0) _y = _y + _dy; 
			_dx = 0;
			_dy = 0;
		}
	}

	void Mover::draw(const DWORD& frame) {
//		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		DWORD a = ((DWORD)(255 * _alpha)) << 24;
		DWORD c1 = (a | 0xffffff) & _background;
		DWORD c2 = (a | 0x999999) & _background;
		if (_lButtonDrag) {
			c1 = (a | 0xffffff) & 0xffccccff;
			c2 = (a | 0x999999) & 0xffcccccc;
		}
		_uim->fillSquare(_x + _dx, _y + _dy, _w, _h, c1, c1, c2, c2);

		if (_border) {
			c1 = (a | 0xffffff) & _border;
			c2 = (a | 0x999999) & _border;
			_uim->drawSquare(_x + _dx, _y + _dy, _w, _h, c1, c1, c2, c2);
		}
	}
}
