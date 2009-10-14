#pragma once

#include "PerformanceTimer.h"


class FPSCounter: public PerformanceTimer
{
private:
	Uint32 _count;
	Uint32 _fps;

	void calculation() {
		DWORD time = getTime();
		if (time >= 1000) {
			_fps = _count;
			_count = 0;
			update();
		}
	}

public:
	FPSCounter():PerformanceTimer() {
		start();
	}

	virtual ~FPSCounter() {
	}

	virtual void start() {
		PerformanceTimer::start();
		_count = 0;
		_fps = 0;
	}

	void count() {
		_count++;
	}

	const Uint32 getFPS() {
		calculation();
		return _fps;
	}
};
