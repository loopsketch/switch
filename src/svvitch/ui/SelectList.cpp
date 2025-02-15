#include "SelectList.h"


namespace ui {
	SelectListItem::SelectListItem(UserInterfaceManagerPtr uim): _uim(uim)
	{
	}

	SelectListItem::~SelectListItem() {
		for (int i = 0; i < _texture.size(); i++) {
			SAFE_RELEASE(_texture[i]);
		}
	}

	void SelectListItem::addText(int width, string text) {
		LPDIRECT3DTEXTURE9 texture = _uim->drawText(L"", 18, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, text);
		addTexture(width, texture);
	}

	void SelectListItem::addTexture(int width, LPDIRECT3DTEXTURE9 texture) {
		_width.push_back(width);
		_texture.push_back(texture);
	}

	int SelectListItem::getWidth() {
		int w = 0;
		for (int i = 0; i < _width.size(); i++) {
			w += _width[i];
		}
		return w;
	}

	int SelectListItem::getHeight() {
		D3DSURFACE_DESC desc;
		int h = 20;
		for (int i = 0; i < _texture.size(); i++) {
			HRESULT hr = _texture[i]->GetLevelDesc(0, &desc);
			if (SUCCEEDED(hr)) {
				if (h < desc.Height) h = desc.Height;
			}
		}
		return h;
	}

	void SelectListItem::draw(int x, int y, DWORD c1, DWORD c2, DWORD c3, DWORD c4) {
		int h = getHeight();
		D3DSURFACE_DESC desc;
		int pos = 0;
		for (int i = 0; i < _texture.size(); i++) {
			int w = _width[i];
			HRESULT hr = _texture[i]->GetLevelDesc(0, &desc);
			_uim->drawTexture(x + pos + (w - desc.Width) / 2, y + (h - desc.Height) / 2, _texture[i], c1, c2, c3, c4);
			pos += w;
		}
	}


	SelectList::SelectList(string name, UserInterfaceManagerPtr uim, int x, int y, int w, int h, float alpha):
		Component(name, uim, x, y, w, h, alpha), MouseReactionUI(this), _itemY(0), _itemHeight(20), _dragKnob(false), _hoverItem(-1), _selectedItem(-1), _listener(NULL)
	{
		_background = 0x99333333;
		_up = new ui::Button(name + "_up", _uim, x + w - 20, y, 20, 20);
		_up->setBackground(0xff9999cc);
		_up->setText(L"��");
		class UpMouseListener: public ui::MouseListener {
			friend class SelectList;
			SelectList& _list;
			UpMouseListener(SelectList& list): _list(list) {
			}

			void buttonDownL() {
				_list.upItems();
			}
		};
		_up->setMouseListener(new UpMouseListener(*this));

		_down = new ui::Button(name + "_down", _uim, x + w - 20, y + h - 20, 20, 20);
		_down->setBackground(0xff9999cc);
		_down->setText(L"��");
		class DownMouseListener: public ui::MouseListener {
			friend class SelectList;
			SelectList& _list;
			DownMouseListener(SelectList& list): _list(list) {
			}

			void buttonDownL() {
				_list.downItems();
			}
		};
		_down->setMouseListener(new DownMouseListener(*this));
		_knob = new ui::Mover(name + "_knob", _uim, x + w - 20, y + 20, 20, h - 40);
		_knob->setBackground(0xff9999cc);
		_knob->setMovingBounds(0, 0, 0, h - 40);
	}

	SelectList::~SelectList(void) {
		removeAll();
//		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		SAFE_DELETE(_listener);
		SAFE_DELETE(_up);
		SAFE_DELETE(_down);
		SAFE_DELETE(_knob);
	}

	void SelectList::upItems() {
		int allItemHeight = _itemHeight * _items.size();
		if (_h < allItemHeight) {
			int ny = _knob->getMY();
			int nd = _knob->getMH() / _items.size();
			if (ny > nd) {
				_knob->setMY((ny - nd) / nd * nd);
			} else {
				_knob->setMY(0);
			}
		}
	}

	void SelectList::downItems() {
		int allItemHeight = _itemHeight * _items.size();
		if (_h < allItemHeight) {
			int ny = _knob->getMY();
			int mh = _knob->getMH();
			int nd = _knob->getMH() / _items.size();
			if (ny < mh - _knob->getHeight()) {
				_knob->setMY((ny + nd) / nd * nd);
			} else {
				_knob->setMY(mh - _knob->getHeight());
			}
		}
	}

	void SelectList::selectBefore() {
		if (_items.size() > 0) {
			if (_selectedItem > 0) {
				_selectedItem--;
			} else {
				_selectedItem = _items.size() - 1;
			}
			if (_listener) _listener->selected();
			visibleSelected();
		}
	}

	void SelectList::selectNext() {
		if (_items.size() > 0) {
			if (_selectedItem < _items.size() - 1) {
				_selectedItem++;
			} else {
				_selectedItem = 0;
			}
			if (_listener) _listener->selected();
			visibleSelected();
		}
	}

	void SelectList::visibleSelected() {
		int y = _itemY + _selectedItem * _itemHeight;
		if (y < 0) {
			_itemY = -_selectedItem * _itemHeight;
		} else if (y > _h - _itemHeight) {
			_itemY = _h - _itemHeight - _selectedItem * _itemHeight;
		}
	}

	void SelectList::removeAll() {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		for (vector<SelectListItemPtr>::iterator it = _items.begin(); it != _items.end(); it++) SAFE_DELETE(*it);
		_items.clear();
		_selectedItem = -1;
		_itemY = 0;
	}

	void SelectList::addItem(const SelectListItemPtr item) {
		if (item) {
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			_items.push_back(item);
			if (_itemHeight < item->getHeight()) _itemHeight = item->getHeight();
			int mh = _h - 40;
			int allItemH = _itemHeight * _items.size();
			if (mh < _h) {
				int nh = mh * _h / allItemH;
				_knob->setSize(20, nh);
				_knob->setEnabled(true);
			} else {
				_knob->setEnabled(false);
			}
		}
	}

	int SelectList::getSelectedIndex() {
		return _selectedItem;
	}

	void SelectList::setSelected(int i) {
		_selectedItem = i;
		visibleSelected();
		if (_listener) _listener->selected();
	}

	void SelectList::process(const DWORD& frame) {
		_hoverItem = -1;
		int x = _mouseX - (_x + _w - 20);
		int y = _mouseY - (_y + 21);
		int h = _h - 42;
		if (_mouseOver) {
			if (_mouseZ > 0) {
				upItems();
				_mouseZ -= 120;
				if (_mouseZ < 0)_mouseZ = 0;
			} else if (_mouseZ < 0) {
				downItems();
				_mouseZ += 120;
				if (_mouseZ > 0)_mouseZ = 0;
			}
			if (x < 0) {
				_hoverItem = ((_mouseY - _y) - _itemY) / _itemHeight;
				if (_lButtonUp) {
					_selectedItem = _hoverItem;
					visibleSelected();
					if (_listener) _listener->selected();
				}
			}
		}

		int allItemH = _itemHeight * _items.size();
		if (_h < allItemH) {
			int mh = _h - 40;
			_itemY = (_h - allItemH) * _knob->getMY() / (_knob->getMH() - _knob->getHeight());
		}
	}

	void SelectList::draw(const DWORD& frame) {
		DWORD a = ((DWORD)(255 * _alpha)) << 24;
		DWORD c1 = (a | 0xffffff) & _background;
		DWORD c2 = (a | 0x999999) & _background;
		_uim->fillSquare(_x, _y, _w, _h, c1, c1, c2, c2);
		c1 = (a | 0xffffff) & 0xccffffff;
		c2 = (a | 0x999999) & 0xcccccccc;
		int h = _h - 42;
		_uim->drawSquare(_x + _w - 20, _y + 21, 20, h, c1, c1, c2, c2);

		LPDIRECT3DDEVICE9 device = _uim->get3DDevice();
		device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
		RECT scissorRect;
		device->GetScissorRect(&scissorRect);
		RECT rect = {_x, _y, _x + _w, _y + _h};
		device->SetScissorRect(&rect);
		int y = 0;
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			for (vector<SelectListItemPtr>::iterator it = _items.begin(); it != _items.end(); it++) {
				SelectListItemPtr item = *it;
//				int w = desc.Width;
				int h = item->getHeight();
				if (_itemY + y + h >= 0) {
					if (y == _selectedItem * _itemHeight) {
						c1 = (a | 0xffffff) & 0xccffcc33;
						c2 = (a | 0xcccccc) & 0xcccc9900;
						_uim->fillSquare(_x, _y + _itemY + y, _w - 20, _itemHeight, c1, c1, c2, c2);
					}
					if (y == _hoverItem * _itemHeight) {
						c1 = (a | 0xffffff) & 0xccffffff;
						c2 = (a | 0xcccccc) & 0xccffffff;
						_uim->drawSquare(_x, _y + _itemY + y, _w - 20, _itemHeight, c1, c1, c2, c2);
					}
					float alpha = 0.3f + _alpha;
					if (alpha > F(1)) alpha = F(1);
					c1 = (((DWORD)(255 * alpha)) << 24) | 0xffffff;
					c2 = (((DWORD)(255 * alpha)) << 24) | 0xcccccc;
					int dy = (_itemHeight - h) / 2;
					item->draw(_x, _y + _itemY + y + dy, c1, c1, c2, c2);
				}
				if (_itemY + y > _h) break;
				y += _itemHeight;
			}
		}
		device->SetScissorRect(&scissorRect);
		device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

//		if (_dragKnob) {
//			c1 = (a | 0xffffff) & 0xccff9999;
//			c2 = (a | 0x999999) & 0xcccc6666;
//		} else {
//			c1 = (a | 0xffffff) & 0xcc333366;
//			c2 = (a | 0x999999) & 0xcc333366;
//		}
//		_uim->fillSquare(_x + _w - 20, _y + 21, 20, h, c1, c1, c2, c2);

		int mh = _itemHeight * _items.size();
		int vy = 0;
		int vh = 0;
		if (_h < mh) {
			vy = h * -_itemY / mh;
			vh = h * (-_itemY + _h) / mh - vy;
		}
		if (vh > 0) {
			c1 = (a | 0xffffff) & 0xcc99cccc;
			c2 = (a | 0x999999) & 0xcc66cccc;
//			_uim->fillSquare(_x + _w - 20, _y + 21 + vy, 20, vh, c1, c1, c2, c2);
		}
//		_uim->debugText(_x, _y, Poco::format("%0.3hf %d %d", F(-_itemY + _h) / mh, vh, h));
//		_uim->debugText(_x, _y, Poco::format("%d %d %d", _x, _y, (int)(_knob->getHeight() / _items.size())));
	}

	void SelectList::setSelectedListener(SelectedListenerPtr listener) {
		SAFE_DELETE(_listener);
		_listener = listener;
	}
}
