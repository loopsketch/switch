#include "Mover.h"

namespace ui {
	Mover::Mover(string name, UserInterfaceManagerPtr uim, int x, int y, int w, int h, float alpha):
		Component(name, uim, x, y, w, h, alpha), MouseReactionUI(this)
	{
	}

	Mover::~Mover(void) {
		_log.information("delete Mover");
//		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
//		SAFE_RELEASE(_text);
	}
}