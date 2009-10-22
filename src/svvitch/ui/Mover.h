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

	public:
		Mover(string name, UserInterfaceManagerPtr uim, int x = 0, int y = 0, int w = 0, int h = 0, float alpha= 1.0f);

		virtual ~Mover(void);
	};

	typedef Mover* MoverPtr;
}
