#pragma once

#include <Poco/Mutex.h>
#include "MouseReactionUI.h"

using std::string;
using std::wstring;


namespace ui {
	/**
	 * ƒ{ƒ^ƒ“
	 */
	class Button: public Component, public MouseReactionUI
	{
	private:
		Poco::FastMutex _lock;
		LPDIRECT3DTEXTURE9 _text;
		int _dy;

	public:
		Button(string name, UserInterfaceManagerPtr uim, int x = 0, int y = 0, int w = 0, int h = 0, float alpha= 1.0f);

		virtual ~Button(void);

		void setText(const string& text);

		void setText(const wstring& text);

		void setTexture(const LPDIRECT3DTEXTURE9 texture);

		virtual void process(const DWORD& frame);

		virtual void draw(const DWORD& frame);
	};

	typedef Button* ButtonPtr;
}
