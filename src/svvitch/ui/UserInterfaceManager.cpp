#include "UserInterfaceManager.h"
#include "Component.h"
#include "MouseReactionUI.h"


namespace ui {
	UserInterfaceManager::UserInterfaceManager(Renderer& renderer):
		_log(Poco::Logger::get("")), _renderer(renderer)
	{
		_cursor = _renderer.createTexture("cursor.png");

//		RECT rect = _renderer.config()->subRect;
//		int x = rect.right / 2;
//		int y = rect.bottom / 2;
		POINT p;
		::GetCursorPos(&p);
		int x = p.x;
		int y = p.y;
		for (int i = 0; i < 10; i++) {
			_mousePoint[i].x = F(x);
			_mousePoint[i].y = F(y);
			_mousePoint[i].y = F(y);
			_mousePoint[i].y = F(y);
			_mousePoint[i].y = F(y);
			_mousePoint[i].z = 0.0f;
			_mousePoint[i].rhw = 1.0f;
			_mousePoint[i].size = 16.0f;
			_mousePoint[i].color = ((0xcc * (10 - i) / 10) << 24) | ((0xcc + 0x33 * (10 - i) / 10) << 16) | ((0xcc + 0x33 * (10 - i) / 10) << 8) | 0xff;
		}
//		ZeroMemory(&_dims, sizeof(DIMOUSESTATE2));
		_mouseX = x;
		_mouseY = y;
		_mouseZ = 0;
		_lButton = false;
		_mButton = false;
		_rButton = false;
		
		_himc = ::ImmGetContext(_renderer.getWindowHandle());
	}

	UserInterfaceManager::~UserInterfaceManager() {
//		initialize();

		::ImmReleaseContext(_renderer.getWindowHandle(), _himc);
		SAFE_RELEASE(_cursor);
	}


	void UserInterfaceManager::initialize() {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		_components.clear();
	}

	LPDIRECT3DDEVICE9 UserInterfaceManager::get3DDevice() {
		return _renderer.get3DDevice();
	}

	const int UserInterfaceManager::getMouseX() const {
		return _mouseX;
	}
	const int UserInterfaceManager::getMouseY() const {
		return _mouseY;
	}

	void UserInterfaceManager::drawLine(const int x1, const int y1, const DWORD c1, const int x2, const int y2, const DWORD c2) {
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		VERTEX v[] = {
			{F(x1), F(y1), F(0), F(1), c1, F(0), F(0)},
			{F(x2), F(y2), F(0), F(1), c2, F(0), F(0)}
		};
		device->DrawPrimitiveUP(D3DPT_LINELIST, 1, v, sizeof(VERTEX));
	}

	void UserInterfaceManager::drawSquare(const int x, const int y, const int w, const int h, const DWORD c1, const DWORD c2, const DWORD c3, const DWORD c4) {
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		int x2 = x + w - 1;
		int y2 = y + h - 1;
		VERTEX v[] = {
			{F(x  - 0.0), F(y  - 0.0), F(0), F(1), c1, F(0), F(0)},
			{F(x2 - 0.0), F(y  - 0.0), F(0), F(1), c2, F(0), F(0)},

			{F(x2 - 0.0), F(y  - 0.0), F(0), F(1), c2, F(0), F(0)},
			{F(x2 - 0.0), F(y2 - 0.0), F(0), F(1), c4, F(0), F(0)},

			{F(x2 - 0.0), F(y2 - 0.0), F(0), F(1), c4, F(0), F(0)},
			{F(x  + 0.0), F(y2 - 0.0), F(0), F(1), c3, F(0), F(0)},

			{F(x  - 0.0), F(y2 - 0.0), F(0), F(1), c3, F(0), F(0)},
			{F(x  - 0.0), F(y  + 0.0), F(0), F(1), c1, F(0), F(0)}
		};
		device->DrawPrimitiveUP(D3DPT_LINELIST, 4, v, sizeof(VERTEX));
	}

	void UserInterfaceManager::fillSquare(const int x, const int y, const int w, const int h, const DWORD c1, const DWORD c2, const DWORD c3, const DWORD c4) {
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		int x2 = x + w - 1;
		int y2 = y + h - 1;
		VERTEX v[] = {
			{F(x  - 0.0), F(y  - 0.0), 0.0f, 1.0f, c1, 0, 0},
			{F(x2 - 0.0), F(y  - 0.0), 0.0f, 1.0f, c2, 1, 0},
			{F(x  - 0.0), F(y2 - 0.0), 0.0f, 1.0f, c3, 0, 1},
			{F(x2 - 0.0), F(y2 - 0.0), 0.0f, 1.0f, c4, 1, 1}
		};
		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, VERTEX_SIZE);
	}

	LPDIRECT3DTEXTURE9 UserInterfaceManager::drawText(const wstring fontFamily, const int fontSize, const DWORD c1, const DWORD c2, const int w1, const DWORD c3, const int w2, const DWORD c4, const string text) {
		LPDIRECT3DTEXTURE9 texture = _renderer.createTexturedText(fontFamily, fontSize, c1, c2, w1, c3, w2, c4, text);
		return texture;
		//renderer.addCachedTexture(name, texture);
	}

	void UserInterfaceManager::drawTexture(const int x, const int y, const LPDIRECT3DTEXTURE9 texture, const D3DCOLOR c1, const D3DCOLOR c2, const D3DCOLOR c3, const D3DCOLOR c4) const {
		_renderer.drawTexture(x, y, texture, c1, c2, c3, c4);
	}

	void UserInterfaceManager::drawTexture(const int x, const int y, const int w, const int h, const LPDIRECT3DTEXTURE9 texture, const D3DCOLOR c1, const D3DCOLOR c2, const D3DCOLOR c3, const D3DCOLOR c4) const {
		_renderer.drawTexture(x, y, w, h, texture, c1, c2, c3, c4);
	}

	void UserInterfaceManager::addComponent(std::string name, ComponentPtr c) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		_components[name] = c;
	}

	ComponentPtr UserInterfaceManager::getComponent(std::string name) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		if (_components.find(name) != _components.end()) {
			return _components.find(name)->second;
		}
//		try {
//			return _components.find(name)->second;
//		} catch (Poco::NotFoundException ex) {
//		_log.debug(Poco::format("component not found: %s", name));
//		}
		return NULL;
	}

	void UserInterfaceManager::removeComponent(std::string name) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		_components.erase(name);
	}

	void UserInterfaceManager::process(const DWORD& frame) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		if (_mouseUpdated) {
			for (Poco::HashMap<string, ComponentPtr>::Iterator it = _components.begin(); it != _components.end(); it++) {
				ui::MouseReactionUIPtr mui = dynamic_cast<ui::MouseReactionUIPtr>(it->second);
				if (mui) mui->processMouse(_mouseX, _mouseY, _mouseZ, _lButton, _rButton);
			}
			_mouseZ = 0;
			_mouseUpdated = false;
		}

		for (Poco::HashMap<string, ComponentPtr>::Iterator it = _components.begin(); it != _components.end(); it++) {
			it->second->preprocess(frame);
		}
		for (Poco::HashMap<string, ComponentPtr>::Iterator it = _components.begin(); it != _components.end(); it++) {
			it->second->process(frame);
		}
		for (Poco::HashMap<string, ComponentPtr>::Iterator it = _components.begin(); it != _components.end(); it++) {
			it->second->postprocess(frame);
			ui::MouseReactionUIPtr mui = dynamic_cast<ui::MouseReactionUIPtr>(it->second);
			if (mui) mui->postprocess(frame);
		}
	}

	void UserInterfaceManager::draw(const DWORD& frame) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		for (Poco::HashMap<string, ComponentPtr>::Iterator it = _components.begin(); it != _components.end(); it++) {
			it->second->draw(frame);
		}

		float f = sin(fmod((float)frame, PI));
		for (int i = 0; i < 9; i++) {
			_mousePoint[9 - i].x = _mousePoint[9 - i].x + (_mousePoint[8 - i].x - _mousePoint[9 - i].x) / 2;
			_mousePoint[9 - i].y = _mousePoint[9 - i].y + (_mousePoint[8 - i].y - _mousePoint[9 - i].y) / 2;
//			_mousePoint[9 - i].y = _mousePoint[9 - i].y + (_mousePoint[8 - i].y - _mousePoint[9 - i].y) / 2 + 4 * (f - 0.5f);
			_mousePoint[9 - i].size = _mousePoint[8 - i].size;
		}
		_mousePoint[0].x = F(_mouseX);
		_mousePoint[0].y = F(_mouseY);
		_mousePoint[0].size = 4 + 12 * f;

		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		device->SetFVF(VERTEX_FVF_P);
		device->SetRenderState(D3DRS_POINTSPRITEENABLE, TRUE);
		device->SetTexture(0, _cursor);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->DrawPrimitiveUP(D3DPT_POINTLIST, 10, _mousePoint, sizeof(VERTEX_P));
		device->SetTexture(0, NULL);
		device->SetFVF(VERTEX_FVF);
//		debugText(0, 0, Poco::format("mouse: %d,%d,%d [%s] [%s]", _mouseX, _mouseY, _mouseZ, string(_lButton?"L":" "), string(_rButton?"R":" ")));
	}


	void UserInterfaceManager::notifyButtonDownL(const int x, const int y) {
		_mouseX = x;
		_mouseY = y;
		_lButton = true;
		_mouseUpdated = true;
	}

	void UserInterfaceManager::notifyButtonUpL(const int x, const int y) {
		_mouseX = x;
		_mouseY = y;
		_lButton = false;
		_mouseUpdated = true;
	}

	void UserInterfaceManager::notifyButtonDownM(const int x, const int y) {
		_mouseX = x;
		_mouseY = y;
		_mButton = true;
		_mouseUpdated = true;
	}

	void UserInterfaceManager::notifyButtonUpM(const int x, const int y) {
		_mouseX = x;
		_mouseY = y;
		_mButton = false;
		_mouseUpdated = true;
	}

	void UserInterfaceManager::notifyButtonDownR(const int x, const int y) {
		_mouseX = x;
		_mouseY = y;
		_rButton = true;
		_mouseUpdated = true;
	}

	void UserInterfaceManager::notifyButtonUpR(const int x, const int y) {
		_mouseX = x;
		_mouseY = y;
		_rButton = false;
		_mouseUpdated = true;
	}

	void UserInterfaceManager::notifyMouseMove(const int x, const int y) {
		_mouseX = x;
		_mouseY = y;
		_mouseUpdated = true;
	}

	void UserInterfaceManager::notifyMouseWheel(const int wheel) {
		_mouseZ = wheel;
		_mouseUpdated = true;
	}


	void UserInterfaceManager::debugText(const int x, const int y, const string& text) {
		_renderer.beginFont(L"", 12);
		_renderer.drawFont(x, y, 0xffffff, 0x000000, text);
		_renderer.endFont();
	}
}
