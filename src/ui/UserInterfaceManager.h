#pragma once

#include "../Renderer.h"

#include <vector>
#include <Poco/format.h>
#include <Poco/HashMap.h>
#include <Poco/Logger.h>

using std::vector;


namespace ui {

	class Component;

	struct VERTEX_P {
		float x, y, z;
		float rhw;
		float size;
		D3DCOLOR color;
	};

	inline DWORD GET_ADDRESS(FLOAT f) { return *((DWORD*)&f); }

	#define VERTEX_FVF_P	(D3DFVF_XYZRHW | D3DFVF_PSIZE | D3DFVF_DIFFUSE)

	/**
	 * UserInterfaceManager
	 **/
	class UserInterfaceManager {
	private:
		Poco::Logger& _log;
		Renderer& _renderer;

		bool _mouseUpdated;
		int _mouseX;
		int _mouseY;
		int _mouseZ;
		bool _lButton;
		bool _mButton;
		bool _rButton;

		LPDIRECT3DTEXTURE9 _cursor;
		VERTEX_P _mousePoint[10];

		Poco::HashMap<string, Component*> _components;


	public:
		UserInterfaceManager(Renderer& renderer);

		virtual ~UserInterfaceManager();

		void initialize();

		LPDIRECT3DDEVICE9 get3DDevice();

		const int getMouseX() const;
		const int getMouseY() const;

		void drawLine(const int x1, const int y1, const DWORD c1, const int x2, const int y2, const DWORD c2);

		void drawSquare(const int x, const int y, const int w, const int h, const DWORD c1, const DWORD c2, const DWORD c3, const DWORD c4);

		void fillSquare(const int x, const int y, const int w, const int h, const DWORD c1, const DWORD c2, const DWORD c3, const DWORD c4);

		LPDIRECT3DTEXTURE9 drawText(const wstring fontFamily, const int fontSize, const DWORD c1, const DWORD c2, const int w1, const DWORD c3, const int w2, const DWORD c4, const string text);

		void drawTexture(const int x, const int y, const LPDIRECT3DTEXTURE9 texture, const D3DCOLOR c1, const D3DCOLOR c2, const D3DCOLOR c3, const D3DCOLOR c4) const;

		void drawTexture(const int x, const int y, const int w, const int h, const LPDIRECT3DTEXTURE9 texture, const D3DCOLOR c1, const D3DCOLOR c2, const D3DCOLOR c3, const D3DCOLOR c4) const;

		void addComponent(std::string name, Component* c);
		Component* getComponent(std::string name) const;
		void removeComponent(std::string name);

		void processMouse(const int x, const int y, const int lButton, const int rButton);

		virtual void process(const DWORD& frame);

		virtual void draw(const DWORD& frame);


		void notifyButtonDownL(const int x, const int y);
		void notifyButtonUpL(const int x, const int y);

		void notifyButtonDownM(const int x, const int y);
		void notifyButtonUpM(const int x, const int y);

		void notifyButtonDownR(const int x, const int y);
		void notifyButtonUpR(const int x, const int y);

		void notifyMouseMove(const int x, const int y);

		void notifyMouseWheel(const int wheel);

		void debugText(const int x, const int y, const string& text);
	};

	typedef UserInterfaceManager* UserInterfaceManagerPtr;
}
