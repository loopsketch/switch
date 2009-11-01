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

	public:
		Mover(string name, UserInterfaceManagerPtr uim, int x = 0, int y = 0, int w = 0, int h = 0, float alpha= 1.0f);

		virtual ~Mover(void);

		/** �ړ��ʒu��ݒ� */
		void setMX(int mx);
		void setMY(int my);

		/** �ړ��ʒu�A�̈�̐ݒ� */
		void setMovingBounds(int x, int y, int w, int h);

		/** �ړ��ʒu�A�̈���擾 */
		int getMX();
		int getMY();
		int getMW();
		int getMH();

		/** ���� */
		virtual void process(const DWORD& frame);

		/** �`�� */
		virtual void draw(const DWORD& frame);
	};

	typedef Mover* MoverPtr;
}
