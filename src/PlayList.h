#pragma once

#include <vector>
#include <Poco/Logger.h>
#include "PlayListItem.h"

using std::string;
using std::vector;


class PlayList
{
friend class Workspace;
private:
	Poco::Logger& _log;

	string _id;
	string _name;
	vector<PlayListItemPtr> _items;

	void add(const PlayListItemPtr media) {
		_items.push_back(media);
	}

public:
	PlayList(const string id, const string name): _log(Poco::Logger::get("")), _id(id), _name(name) {
	}

	~PlayList() {
		for (vector<PlayListItemPtr>::iterator it = _items.begin(); it != _items.end(); it++) SAFE_DELETE(*it);
		_items.clear();
//		_log.information(Poco::format("delete PlayList: %s", _name));
	}

	const string& id() const {
		return _id;
	}

	const string& name() const {
		return _name;
	}

	const int itemCount() const {
		return _items.size();
	}

	const vector<PlayListItemPtr>& items() const {
		return _items;
	}
};

typedef PlayList* PlayListPtr;