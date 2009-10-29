#pragma once

#include <string>
#include <vector>
#include <Poco/format.h>
#include <Poco/Logger.h>

#include "UserInterfaceManager.h"

using std::vector;
using std::string;


namespace ui {

	class Component
	{
	private:
		vector<Component*> _child;

	protected:
		Poco::Logger& _log;
		string _name;
		UserInterfaceManagerPtr _uim;

		bool _enabled;
		int _x, _y, _w, _h;
		float _alpha;

		DWORD _color;
		DWORD _background;
		DWORD _border;

	public:
		Component(string name, UserInterfaceManagerPtr uim, int x, int y, int w, int h, float alpha):
			_log(Poco::Logger::get("")), _name(name), _uim(uim), _enabled(true), _x(x), _y(y), _w(w), _h(h), _alpha(alpha),
			_color(0xccffffff), _background(0xcc000000), _border(0xccffffff)
		{
			_uim->addComponent(name, this);
		}

		virtual ~Component(void) {
//			_log.information("delete Component");
			_uim->removeComponent(_name);
		}

		UserInterfaceManagerPtr getUserInterfaceManager() {
			return _uim;
		}

		/** �L�����ǂ��� */
		bool getEnabled() const {
			return _enabled;
		}

		void setEnabled(bool enabled) {
			_enabled = enabled;
		}

		/** �J���[�ݒ� */
		void setColor(const DWORD color) {
			_color = color;
		}

		/** �w�i�F�ݒ� */
		void setBackground(const DWORD color) {
			_background = color;
		}

		/** �T�C�Y��ݒ� */
		void setSize(int w, int h) {
			_w = w;
			_h = h;
		}

		int getY() {
			return _y;
		}

		int getWidth() {
			return _w;
		}

		int getHeight() {
			return _h;
		}

		/** �T�u�R���|�[�l���g�ǉ� */
		void add(Component* ui) {
			_child.push_back(ui);
		}

		/** �w����W���R���|�[�l���g�����ǂ��� */
		bool contains(int x, int y) {
			return x >= _x && y >= _y && x <= _x + _w && y <= _y + _h;
		}

		/** process�̑O�����B1�t���[����1�x������������� */
		virtual void preprocess(const DWORD& frame) {
		}

		/** 1�t���[����1�x������������� */
		virtual void process(const DWORD& frame) {
		}

		/** process�̌㏈���B1�t���[����1�x������������� */
		virtual void postprocess(const DWORD& frame) {
		}

		/** �`�� */
		virtual void draw(const DWORD& frame) {
		}

	};

	typedef Component* ComponentPtr;
}
