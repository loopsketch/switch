#pragma once

#include <Poco/Logger.h>
#include <Poco/SharedPtr.h>
#include "MediaItem.h"


class PlayListItem
{
private:
	Poco::Logger& _log;
	MediaItemPtr _media;
	string _next;
	string _transition;

public:
	PlayListItem(const MediaItemPtr media, const string& next, const string transition):
	  _log(Poco::Logger::get("")), _media(media), _next(next), _transition(transition)
	{
	}

	~PlayListItem() {
		// _media‚ÍŠJ•ú‚µ‚È‚¢
	}

	const MediaItemPtr media() const {
		return _media;
	}

	const string& next() const {
		return _next;
	}

	const string& transition() const {
		return _transition;
	}
};

typedef PlayListItem* PlayListItemPtr;