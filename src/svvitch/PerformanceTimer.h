#pragma once

#include "Common.h"

/**
 * パフォーマンスタイマクラス.
 * パフォーマンスタイマを用いた高精度な時間計測機能を提供します
 */
class PerformanceTimer
{
private:
	LARGE_INTEGER _freq;
	LARGE_INTEGER _start;
	LARGE_INTEGER _current;
	bool _enabled;

public:
	PerformanceTimer(): _enabled(false) {
		::QueryPerformanceFrequency(&_freq);
	}

	virtual ~PerformanceTimer() {
	}

	virtual void start() {
		::QueryPerformanceCounter(&_start);
		_current = _start;
		_enabled = true;
	}

	const DWORD getTime() {
		if (_enabled) {
			::QueryPerformanceCounter(&_current);
			return (DWORD)((_current.QuadPart - _start.QuadPart) * 1000 / _freq.QuadPart);
		}
		return 0;
	}

	void update() {
		_start = _current;
	}
};
