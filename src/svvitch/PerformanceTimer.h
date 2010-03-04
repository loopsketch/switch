#pragma once

#include "Common.h"

/**
 * 
 */
class PerformanceTimer
{
private:
	LARGE_INTEGER _freq;
	LARGE_INTEGER _start;
	LARGE_INTEGER _current;

public:
	PerformanceTimer() {
		::QueryPerformanceFrequency(&_freq);
	}

	virtual ~PerformanceTimer() {
	}

	virtual void start() {
		::QueryPerformanceCounter(&_start);
		_current = _start;
	}

	const DWORD getTime() {
		::QueryPerformanceCounter(&_current);
		return (DWORD)((_current.QuadPart - _start.QuadPart) * 1000 / _freq.QuadPart);
	}

	void update() {
		_start = _current;
	}
};
