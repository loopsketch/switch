#include "MFContent.h"
#include <Poco/UnicodeConverter.h>


MFContent::MFContent(Renderer& renderer, int splitType): Content(renderer, splitType) {
}

MFContent::~MFContent() {
	initialize();
}

void MFContent::initialize() {
	close();
	HRESULT hr;
	hr = MFCreateMediaSession(NULL, &_session);
}

/** ファイルをオープンします */
bool MFContent::open(const MediaItemPtr media, const int offset) {
	initialize();
	if (media->files().empty()) return false;

	HRESULT hr;
	return false;
}

/**
 * 再生
 */
void MFContent::play() {
}

/**
 * 停止
 */
void MFContent::stop() {
	_playing = false;
}

/**
 * 再生中かどうか
 */
const bool MFContent::playing() const {
	return _playing;
}

const bool MFContent::finished() {
	return _finished;
}

/** ファイルをクローズします */
void MFContent::close() {
	stop();
}

void MFContent::process(const DWORD& frame) {
}

void MFContent::draw(const DWORD& frame) {
}
