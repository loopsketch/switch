#pragma once

#include <Poco/Mutex.h>
#include "MouseReactionUI.h"
#include "SelectedListener.h"
#include "Button.h"
#include "Mover.h"

#include <vector>

using std::string;
using std::vector;


namespace ui {
	/**
	 * SelectListのアイテムクラス
	 */
	class SelectListItem
	{
	private:
		UserInterfaceManagerPtr _uim;
		vector<int> _width;
		vector<LPDIRECT3DTEXTURE9> _texture;

	public:
		SelectListItem(UserInterfaceManagerPtr uim);

		virtual ~SelectListItem();

		void addText(int width, string text);

		void addTexture(int width, LPDIRECT3DTEXTURE9 texture);

		int getWidth();

		int getHeight();

		void draw(int x, int y, DWORD c1, DWORD c2, DWORD c3, DWORD c4);
	};

	typedef SelectListItem* SelectListItemPtr;


	/**
	 * SelectListクラス
	 */
	class SelectList: public Component, public MouseReactionUI
	{
	private:
		Poco::FastMutex _lock;
		ui::ButtonPtr _up;
		ui::ButtonPtr _down;
		ui::MoverPtr _knob;
		vector<SelectListItemPtr> _items;
		int _itemY;
		int _itemHeight;
		bool _dragKnob;
		int _hoverItem;
		int _selectedItem;
		SelectedListenerPtr _listener;

		void upItems();

		void downItems();

	public:
		SelectList(string name, UserInterfaceManagerPtr uim, int x = 0, int y = 0, int w = 0, int h = 0, float alpha= 1.0f);

		virtual ~SelectList();

		void selectBefore();

		void selectNext();

		void visibleSelected();

		void removeAll();

		void addItem(const SelectListItemPtr item);

		int getSelectedIndex();

		void setSelected(int i);

		virtual void process(const DWORD& frame);

		virtual void draw(const DWORD& frame);

		virtual void setSelectedListener(SelectedListenerPtr listener);
	};

	typedef SelectList* SelectListPtr;
}
