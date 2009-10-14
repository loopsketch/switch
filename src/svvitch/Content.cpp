#include "Content.h"

#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>


Content::Content(Renderer& renderer, float x, float y, float w, float h):
	_log(Poco::Logger::get("")), _renderer(renderer), _duration(0), _current(0), _x(x), _y(y), _w(w), _h(h), _playing(false), _media(NULL)
{
}

Content::~Content() {
	initialize();
	_properties.clear();
}

void Content::initialize() {
}

bool Content::open(const MediaItemPtr media, const int offset) {
	_media = media;
	return true;
}

const MediaItemPtr Content::opened() const {
	return _media;
}

void Content::play() {
	_playing = true;
}

void Content::stop() {
	_playing = false;
}

/**
 * 再生中かどうか
 */
const bool Content::playing() const {
	return _playing;
}

const bool Content::finished() {
	return false;
}

/** ファイルをクローズします */
void Content::close() {
	_media = NULL;
}

/** キー入力の通知 */
void Content::notifyKey(const int keycode, const bool shift, const bool ctrl) {
	_keycode = keycode;
	_shift = shift;
	_ctrl = ctrl;
}

/** 1フレームに1度だけ処理される */
void Content::process(const DWORD& frame) {
}

/** 描画 */
void Content::draw(const DWORD& frame) {
}

/** プレビュー描画 */
void Content::preview(const DWORD& frame) {
}

/**
 * 現在のフレーム
 */
const int Content::current() const {
	return _current;
}

/**
 * 長さ(フレーム数)
 */
const int Content::duration() const {
	return _duration;
}


void Content::setPosition(float x, float y) {
	_x = x;
	_y = y;
}

void Content::getPosition(float& x, float& y) {
	x = _x;
	y = _y;
}

void Content::setBounds(float w, float h) {
	_w = w;
	_h = h;
}

const bool Content::contains(float x, float y) const {
	return x >= _x && y >= _y && x <= _x + _w && y <= _y + _h;
}

void Content::set(const string& key, const string& value) {
	if (_properties.find(key) != _properties.end()) _properties.erase(key);
	_properties[key] = value;
}

void Content::set(const string& key, const float& value) {
	string s;
	Poco::NumberFormatter::append(s, value);
	set(key, s);
}

const string& Content::get(const string& key) const {
	HashMap<string, string>::ConstIterator it = _properties.find(key);
	if (it != _properties.end()) {
		return it->second;
	}
	return NULL_STRING;
}

const float Content::getF(const string& key, const float& defaultValude) const {
	const string value = get(key);
	if (!value.empty()) {
		try {
			return (float)Poco::NumberParser::parseFloat(value);
		} catch (Poco::SyntaxException& ex) {
		}
	}
	return defaultValude;
}
