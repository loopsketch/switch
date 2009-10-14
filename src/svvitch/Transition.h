#pragma once

#include "Content.h"

#include <Poco/Logger.h>
#include <Poco/Mutex.h>


class Transition {
protected:
	Poco::Logger& _log;
	Poco::FastMutex _lock;
	ContentPtr _c1;
	ContentPtr _c2;

public:
	Transition(ContentPtr c1, ContentPtr c2);
	virtual ~Transition();

	virtual void initialize(const DWORD& frame);
	virtual bool process(const DWORD& frame);
};


typedef Transition* TransitionPtr;