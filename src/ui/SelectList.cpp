#include "SelectList.h"


namespace ui {
	SelectList::SelectList(string name, UserInterfaceManagerPtr uim, int x, int y, int w, int h, float alpha):
		Component(name, uim, x, y, w, h, alpha), MouseReactionUI(this), _itemY(0), _itemHeight(20), _dragKnob(false), _hoverItem(-1), _selectedItem(-1), _listener(NULL)
	{
		_background = 0x99333333;
		_up = new ui::Button(name + "_up", _uim, x + w - 20, y, 20, 20);
		_up->setBackground(0xff333366);
		_up->setText(L"▲");
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
		_down->setBackground(0xff333366);
		_down->setText(L"▼");
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
	}

	SelectList::~SelectList(void) {
		remveAll();
		SAFE_DELETE(_listener);
		SAFE_DELETE(_up);
		SAFE_DELETE(_down);
	}

	void SelectList::upItems() {
		int itemHeightTotal = _itemHeight * _items.size();
		if (_h < itemHeightTotal) {
			if (_itemY + _itemHeight > 0) {
				_itemY = 0;
			} else {
				_itemY += _itemHeight;
			}
		}
	}

	void SelectList::downItems() {
		int itemHeightTotal = _itemHeight * _items.size();
		if (_h < itemHeightTotal) {
			int min = _h - itemHeightTotal;
			if (_itemY - _itemHeight < min) {
				_itemY = min;
			} else {
				_itemY -= _itemHeight;
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

	void SelectList::remveAll() {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
//		for (vector<LPDIRECT3DTEXTURE9>::iterator it = _items.begin(); it != _items.end(); it++) SAFE_RELEASE(*it);
		_items.clear();
		_selectedItem = -1;
		_itemY = 0;
	}

	void SelectList::addItem(const LPDIRECT3DTEXTURE9 texture) {
		if (texture) {
			_items.push_back(texture);

			D3DSURFACE_DESC desc;
			HRESULT hr = texture->GetLevelDesc(0, &desc);
			if (SUCCEEDED(hr)) {
				if (_itemHeight < desc.Height) _itemHeight = desc.Height;
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
			if (x > 0 && x < 20 && y > 0 && y < h && _lButtonDown) {
				_dragKnob = true;
			}
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
		if (_lButtonUp) {
			_dragKnob = false;
		}

		if (_dragKnob) {
			// サム内でドラック中
			int mh = _itemHeight * _items.size();
			if (_h < mh) {
				if (y < 0) {
					y = 0;
				} else if (y > h) {
					y = h;
				}
				_itemY = -((mh - _h) * y / (h - 1));
			}
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
			for (vector<LPDIRECT3DTEXTURE9>::iterator it = _items.begin(); it != _items.end(); it++) {
				LPDIRECT3DTEXTURE9 texture = *it;
				D3DSURFACE_DESC desc;
				HRESULT hr = texture->GetLevelDesc(0, &desc);
				if (SUCCEEDED(hr)) {
					int w = desc.Width;
					int h = desc.Height;
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
						_uim->drawTexture(_x, _y + _itemY + y + dy, texture, c1, c1, c2, c2);
					}
					if (_itemY + y > _h) break;
					y += _itemHeight;
				}
			}
		}
		device->SetScissorRect(&scissorRect);
		device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

		if (_dragKnob) {
			c1 = (a | 0xffffff) & 0xccff9999;
			c2 = (a | 0x999999) & 0xcccc6666;
		} else {
			c1 = (a | 0xffffff) & 0xcc333366;
			c2 = (a | 0x999999) & 0xcc333366;
		}
		_uim->fillSquare(_x + _w - 20, _y + 21, 20, h, c1, c1, c2, c2);
	}

	void SelectList::setSelectedListener(SelectedListenerPtr listener) {
		SAFE_DELETE(_listener);
		_listener = listener;
	}
}
