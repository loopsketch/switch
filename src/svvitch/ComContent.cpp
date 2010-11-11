#include "ComContent.h"


ComContent::ComContent(Renderer& renderer, int splitType, float x, float y, float w, float h):
	Content(renderer, splitType, x, y, w, h), _updated(false)
{
}

ComContent::~ComContent() {
}

void ComContent::update() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	_updated = true;
}
