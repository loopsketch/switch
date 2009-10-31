#include "Mover.h"

namespace ui {
	Mover::Mover(string name, UserInterfaceManagerPtr uim, int x, int y, int w, int h, float alpha):
		Component(name, uim, x, y, w, h, alpha), MouseReactionUI(this), _mx(0), _my(0), _mw(w), _mh(h)
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

	int Mover::getMX() {
		return _mx;
	}

	int Mover::getMY() {
		return _my;
	}

	void Mover::process(const DWORD& frame) {
		_mouseOver = (_mouseX >= _x + _mx && _mouseY >= _y + _my && _mouseX <= _x + _mx + _w && _mouseY <= _y + _my + _h);
		if (_enabled && _mouseOver) {
			(_alpha < 0.8f)? _alpha += 0.05f: _alpha = 0.8f;
		} else {
			(_alpha > 0.5f)? _alpha -= 0.05f: _alpha = 0.5f;
		}

		if (_lButtonDrag) {
			if (_mw > 0) {
				int dx = _dragDX;
				if (_mx + dx < 0) {
					dx = -_mx;
				} else if (_mx + dx > _mw) {
					dx = _mw - _mx;
				}
				_mx += dx;
				_dragX += dx;
			}
			if (_mh > 0) {
				int dy = _dragDY;
				if (_my + dy < 0) {
					dy = -_my;
				} else if (_my + dy > _mh) {
					dy = _mh - _my;
				}
				_my += dy;
				_dragY += dy;
			}
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
		_uim->fillSquare(_x + _mx, _y + _my, _w, _h, c1, c1, c2, c2);

		// 
		_uim->drawLine(_dragX, _dragY, 0xffff3333, _dragX, _dragY, 0xff3333);

		if (_border) {
			c1 = (a | 0xffffff) & _border;
			c2 = (a | 0x999999) & _border;
			_uim->drawSquare(_x + _mx, _y + _my, _w, _h, c1, c1, c2, c2);
		}
	}
}
