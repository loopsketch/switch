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
		Component(string name, UserInterfaceManagerPtr uim, int x, int y, int w, int h, float alpha);

		virtual ~Component(void);

		UserInterfaceManagerPtr getUserInterfaceManager();

		/** �L�����ǂ��� */
		bool getEnabled() const;

		void setEnabled(bool enabled);

		/** �J���[�ݒ� */
		void setColor(const DWORD color);

		/** �w�i�F�ݒ� */
		void setBackground(const DWORD color);

		/** �ʒu��ݒ� */
		void setPosition(int x, int y);

		/** �T�C�Y��ݒ� */
		void setSize(int w, int h);

		/** �ʒu���擾 */
		virtual int getX();
		virtual int getY();

		/** �T�C�Y���擾 */
		virtual int getWidth();
		virtual int getHeight();

		/** �T�u�R���|�[�l���g�ǉ� */
		void add(Component* ui);

		/** �w����W���R���|�[�l���g�����ǂ��� */
		bool contains(int x, int y);

		/** process�̑O�����B1�t���[����1�x������������� */
		virtual void preprocess(const DWORD& frame);

		/** 1�t���[����1�x������������� */
		virtual void process(const DWORD& frame);

		/** process�̌㏈���B1�t���[����1�x������������� */
		virtual void postprocess(const DWORD& frame);

		/** �`�� */
		virtual void draw(const DWORD& frame);
	};

	typedef Component* ComponentPtr;
}
