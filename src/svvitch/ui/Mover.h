#pragma once

#include <Poco/Mutex.h>
#include "MouseReactionUI.h"


namespace ui {
	/**
	 * ‹K’èƒGƒŠƒA‚Ì’†‚ğ“®‚©‚¹‚é˜g(Slider“I—˜—p‚ª‚Å‚«‚Ü‚·)
	 */
	class Mover: public Component, public MouseReactionUI
	{
	private:
		Poco::FastMutex _lock;

	public:
		Mover(string name, UserInterfaceManagerPtr uim, int x = 0, int y = 0, int w = 0, int h = 0, float alpha= 1.0f);

		virtual ~Mover(void);

		/** ˆ— */
		virtual void process(const DWORD& frame);

		/** •`‰æ */
		virtual void draw(const DWORD& frame);
	};

	typedef Mover* MoverPtr;
}
