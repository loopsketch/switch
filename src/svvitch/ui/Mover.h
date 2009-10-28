#pragma once

#include <Poco/Mutex.h>
#include "MouseReactionUI.h"


namespace ui {
	/**
	 * �K��G���A�̒��𓮂�����g(Slider�I���p���ł��܂�)
	 */
	class Mover: public Component, public MouseReactionUI
	{
	private:
		Poco::FastMutex _lock;

		int _mx;
		int _my;
		int _mw;
		int _mh;

		int _dx;
		int _dy;

	public:
		Mover(string name, UserInterfaceManagerPtr uim, int x = 0, int y = 0, int w = 0, int h = 0, float alpha= 1.0f);

		virtual ~Mover(void);

		/** �ړ��\�̈�̐ݒ� */
		void setMovingBounds(int x, int y, int w, int h);

		/** ���� */
		virtual void process(const DWORD& frame);

		/** �`�� */
		virtual void draw(const DWORD& frame);
	};

	typedef Mover* MoverPtr;
}
