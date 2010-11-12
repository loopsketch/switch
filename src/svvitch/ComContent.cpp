#include "ComContent.h"


ComContent::ComContent(Renderer& renderer, int splitType, float x, float y, float w, float h):
	Content(renderer, splitType, x, y, w, h)
{
}

ComContent::~ComContent() {
}

void ComContent::invalidateRect(int x, int y, int w, int h) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	_invalidateRects.push(Rect(x, y, w, h));
}

bool ComContent::hasInvalidateRect() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	return !_invalidateRects.empty();
}

Rect ComContent::popInvalidateRect() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	Rect rect = _invalidateRects.front();
	_invalidateRects.pop();
	return rect;
}
