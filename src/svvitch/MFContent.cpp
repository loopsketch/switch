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

/** �t�@�C�����I�[�v�����܂� */
bool MFContent::open(const MediaItemPtr media, const int offset) {
	initialize();
	if (media->files().empty()) return false;

	HRESULT hr;
	return false;
}

/**
 * �Đ�
 */
void MFContent::play() {
}

/**
 * ��~
 */
void MFContent::stop() {
	_playing = false;
}

/**
 * �Đ������ǂ���
 */
const bool MFContent::playing() const {
	return _playing;
}

const bool MFContent::finished() {
	return _finished;
}

/** �t�@�C�����N���[�Y���܂� */
void MFContent::close() {
	stop();
}

void MFContent::process(const DWORD& frame) {
}

void MFContent::draw(const DWORD& frame) {
}
