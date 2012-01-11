#pragma once

#include <vector>
#include <Poco/Logger.h>
#include "PlayListItem.h"

using std::string;
using std::vector;


/**
 * プレイリストクラス.
 * 再生順序を定義したプレイリストを表します
 */
class PlayList
{
friend class Workspace;
private:
	Poco::Logger& _log;

	string _id;
	string _name;
	string _text;
	vector<PlayListItemPtr> _items;

	void add(const PlayListItemPtr media) {
		_items.push_back(media);
	}

public:
	PlayList(const string id, const string name, const string text = ""): _log(Poco::Logger::get("")), _id(id), _name(name), _text(text) {
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

	const string& text() const {
		return _text;
	}

	void text(const string text) {
		_text = text;
	}

	const int itemCount() const {
		return _items.size();
	}

	const vector<PlayListItemPtr>& items() const {
		return _items;
	}
};

typedef PlayList* PlayListPtr;