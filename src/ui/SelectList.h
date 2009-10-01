#pragma once

#include <Poco/Mutex.h>
#include "MouseReactionUI.h"
#include "SelectedListener.h"
#include "Button.h"

#include <vector>

using std::string;
using std::vector;


namespace ui {
	class SelectList: public Component, public MouseReactionUI
	{
	private:
		Poco::FastMutex _lock;
		ui::ButtonPtr _up;
		ui::ButtonPtr _down;
		vector<LPDIRECT3DTEXTURE9> _items;
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

		virtual ~SelectList(void);

		void selectBefore();

		void selectNext();

		void visibleSelected();

		void remveAll();

		void addItem(const LPDIRECT3DTEXTURE9 texture);

		int getSelectedIndex();

		void setSelected(int i);

		virtual void process(const DWORD& frame);

		virtual void draw(const DWORD& frame);

		virtual void setSelectedListener(SelectedListenerPtr listener);
	};

	typedef SelectList* SelectListPtr;
}
