#pragma once

#include "UserInterfaceManager.h"
#include "Component.h"
#include "MouseListener.h"


namespace ui {

	class MouseReactionUI
	{
	private:
		ComponentPtr _component;
		MouseListenerPtr _listener;

	protected:
		int _mouseX;
		int _mouseY;
		int _mouseZ;
		bool _mouseOver;
		bool _lButton;
		bool _lButtonDown;
		bool _lButtonUp;
		bool _rButton;
		bool _rButtonDown;
		bool _rButtonUp;

		int _dragX;
		int _dragY;
		bool _lButtonDrag;
		bool _rButtonDrag;

	public:
		MouseReactionUI(ComponentPtr component):
			_component(component), _listener(NULL), _mouseX(0), _mouseY(0), _mouseZ(0),
			_mouseOver(false), _lButton(false), _rButton(false),
			_dragX(0), _dragY(0), _lButtonDrag(false), _rButtonDrag(false)
		{
		}

		virtual ~MouseReactionUI(void) {
			if (_listener) SAFE_DELETE(_listener);
		}

		/** マウス処理 */
		void processMouse(const int x, const int y, const int z, const int lButton, const int rButton) {
			_mouseX = x;
			_mouseY = y;
			_mouseZ = z;
			_mouseOver = _component->contains(x, y);
			if (_component->getEnabled() && _mouseOver) {
				_lButtonDown = (!_lButton && lButton != 0);
				_lButtonUp = (_lButton && lButton == 0);
				_lButton = lButton != 0;
				_rButtonDown = (!_rButton && rButton != 0);
				_rButtonUp = (_rButton && rButton == 0);
				_rButton = rButton != 0;

				if (_listener) {
					if (_lButtonDown) _listener->buttonDownL();
					if (_lButtonUp) _listener->buttonUpL();
					if (_rButtonDown) _listener->buttonDownR();
					if (_rButtonUp) _listener->buttonUpR();
				}
			} else {
				if (_lButton) {
					_lButtonDown = (!_lButton && lButton != 0);
					_lButtonUp = (_lButton && lButton == 0);
					_lButton = lButton != 0;
				}
				if (_rButton) {
					_rButtonDown = (!_rButton && rButton != 0);
					_rButtonUp = (_rButton && rButton == 0);
					_rButton = rButton != 0;
				}
			}
		}

		/** processの後処理。1フレームに1度だけ処理される */
		void postprocess(const DWORD& frame) {
			if (_lButtonDown) {
				_dragX = _mouseX;
				_dragY = _mouseY;
				_lButtonDrag = true;
			}
			if (_lButtonUp) _lButtonDrag = false;
			if (_rButtonDown) {
				_dragX = _mouseX;
				_dragY = _mouseY;
				_rButtonDrag = true;
			}
			if (_rButtonUp) _rButtonDrag = false;

			// ボタン操作検出のエッジ処理のための後処理
			_lButtonDown = false;
			_lButtonUp = false;
			_rButtonDown = false;
			_rButtonUp = false;
		}

		void setMouseListener(MouseListenerPtr listener) {
			SAFE_DELETE(_listener);
			_listener = listener;
		}
	};

	typedef MouseReactionUI* MouseReactionUIPtr;
}
