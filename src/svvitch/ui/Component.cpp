#include "Component.h"

namespace ui {
	Component::Component(string name, UserInterfaceManagerPtr uim, int x, int y, int w, int h, float alpha):
		_log(Poco::Logger::get("")), _name(name), _uim(uim), _enabled(true), _x(x), _y(y), _w(w), _h(h), _alpha(alpha),
		_color(0xccffffff), _background(0xcc000000), _border(0xccffffff)
	{
		_uim->addComponent(name, this);
	}

	Component::~Component(void) {
//			_log.information("delete Component");
		_uim->removeComponent(_name);
	}

	UserInterfaceManagerPtr Component::getUserInterfaceManager() {
		return _uim;
	}

	bool Component::getEnabled() const {
		return _enabled;
	}

	void Component::setEnabled(bool enabled) {
		_enabled = enabled;
	}

	void Component::setColor(const DWORD color) {
		_color = color;
	}

	void Component::setBackground(const DWORD color) {
		_background = color;
	}

	void Component::setPosition(int x, int y) {
		_x = x;
		_y = y;
	}

	void Component::setX(int x) {
		_x = x;
	}

	void Component::setY(int y) {
		_y = y;
	}

	void Component::setSize(int w, int h) {
		_w = w;
		_h = h;
	}

	int Component::getX() {
		return _x;
	}

	int Component::getY() {
		return _y;
	}

	int Component::getWidth() {
		return _w;
	}

	int Component::getHeight() {
		return _h;
	}

	void Component::add(Component* ui) {
		_child.push_back(ui);
	}

	bool Component::contains(int x, int y) {
		return x >= _x && y >= _y && x <= _x + _w && y <= _y + _h;
	}

	void Component::preprocess(const DWORD& frame) {
	}

	void Component::process(const DWORD& frame) {
	}

	void Component::postprocess(const DWORD& frame) {
	}

	void Component::draw(const DWORD& frame) {
	}
}
