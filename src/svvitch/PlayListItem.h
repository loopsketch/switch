#pragma once

#include <Poco/Logger.h>
#include <Poco/SharedPtr.h>
#include "MediaItem.h"


/**
 * �v���C���X�g�A�C�e���N���X.
 * �v���C���X�g�ɒ�`����Ă���1�A�C�e����\���܂�
 */
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
		// _media�͊J�����Ȃ�
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