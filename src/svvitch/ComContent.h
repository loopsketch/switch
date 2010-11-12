#pragma once

#include <Poco/Mutex.h>

#include "Content.h"


using std::queue;

class Rect
{
private:
	Rect& copy(const Rect& rect) {
		if (this == &rect) return *this;
		x = rect.x;
		y = rect.y;
		w = rect.w;
		h = rect.h;
		return *this;
	}

public:
	int x;
	int y;
	int w;
	int h;

	Rect(int x_, int y_, int w_, int h_): x(x_), y(y_), w(w_), h(h_) {
	}

	Rect(const Rect& rect) {
		copy(rect);
	}

	Rect& operator=(const Rect& rect) {
		return copy(rect);
    }
};


class ComContent: public Content {
private:
protected:
	Poco::FastMutex _lock;
	queue<Rect> _invalidateRects;
	//bool _updated;

	ComContent(Renderer& renderer, int splitType, float x = 0, float y = 0, float w = 0, float h = 0);

	virtual ~ComContent();

	virtual void createComComponents() = 0;

	virtual void releaseComComponents() = 0;

public:
	void invalidateRect(int x, int y, int w, int h);

	bool hasInvalidateRect();

	Rect popInvalidateRect();
};

typedef ComContent* ComContentPtr;