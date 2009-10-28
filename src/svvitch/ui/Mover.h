#pragma once

#include <Poco/Mutex.h>
#include "MouseReactionUI.h"


namespace ui {
	/**
	 * 規定エリアの中を動かせる枠(Slider的利用ができます)
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

		/** 移動可能領域の設定 */
		void setMovingBounds(int x, int y, int w, int h);

		/** 処理 */
		virtual void process(const DWORD& frame);

		/** 描画 */
		virtual void draw(const DWORD& frame);
	};

	typedef Mover* MoverPtr;
}
