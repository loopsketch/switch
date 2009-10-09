#include "Container.h"


Container::Container(Renderer& renderer): Content(renderer), _initialized(true) {
}

Container::~Container() {
	initialize();
}

void Container::initialize() {
	_initialized = true;
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		int count = 0;
		for (vector<ContentPtr>::iterator it = _list.begin(); it != _list.end(); it++) SAFE_DELETE(*it);count++;
		_list.clear();
//		_log.information(Poco::format("Container::initialize() %d", count));
	}
}

void Container::add(ContentPtr c) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	_list.push_back(c);
	c->set("itemNo", _list.size());
	_initialized = false;
}

ContentPtr Container::get(int i) {
//	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (!_initialized) {
		try {
			return _list.at(i);
		} catch (std::out_of_range ex) {
		}
	}
	return NULL;
}

int Container::size() {
	return _list.size();
}

ContentPtr Container::operator[](int i) {
//	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	return get(i);
}

const MediaItemPtr Container::opened() const {
	MediaItemPtr media = NULL;
	for (vector<ContentPtr>::const_iterator it = _list.begin(); it != _list.end(); it++) {
		MediaItemPtr m = (*it)->opened();
		if (!m) return NULL;
		if (!media) media = m;
	}
	return media;
}

void Container::play() {
	for (vector<ContentPtr>::iterator it = _list.begin(); it != _list.end(); it++) (*it)->play();
}

void Container::stop() {
	for (vector<ContentPtr>::iterator it = _list.begin(); it != _list.end(); it++) (*it)->stop();
}

const bool Container::finished() {
	bool finished = true;
	for (vector<ContentPtr>::iterator it = _list.begin(); it != _list.end(); it++) {
		finished &= (*it)->finished();
	}
	return finished;
}

void Container::notifyKey(const int keycode, const bool shift, const bool ctrl) {
	if (!_initialized) {
		for (vector<ContentPtr>::iterator it = _list.begin(); it != _list.end(); it++) (*it)->notifyKey(keycode, shift, ctrl);
	}
}

void Container::process(const DWORD& frame) {
	if (!_initialized) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		for (vector<ContentPtr>::iterator it = _list.begin(); it != _list.end(); it++) (*it)->process(frame);
	}
}

void Container::draw(const DWORD& frame) {
	if (!_initialized) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		for (vector<ContentPtr>::iterator it = _list.begin(); it != _list.end(); it++) (*it)->draw(frame);
	}
}

const int Container::current() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (!_list.empty()) {
		return _list.at(0)->current();
	}
	return 0;
}

const int Container::duration() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (!_list.empty()) {
		return _list.at(0)->duration();
	}
	return 0;
}

void Container::setProperty(const string& key, const string& value) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	for (vector<ContentPtr>::iterator it = _list.begin(); it != _list.end(); it++) (*it)->set(key, value);
}
