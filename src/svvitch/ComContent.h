#pragma once

#include <Poco/Mutex.h>

#include "Content.h"


class ComContent: public Content {
protected:
	Poco::FastMutex _lock;
	bool _updated;

	ComContent(Renderer& renderer, int splitType, float x = 0, float y = 0, float w = 0, float h = 0);

	virtual ~ComContent();

public:
	void update();
};

typedef ComContent* ComContentPtr;