#ifdef USE_FLASH


#include "FlashContent.h"
#include <Poco/UnicodeConverter.h>
//#include <Poco/DateTime.h>
//#include <Poco/Timezone.h>
#include "Utils.h"


FlashContent::FlashContent(Renderer& renderer, float x, float y, float w, float h): Content(renderer, x, y, w, h)
{
	initialize();
}

FlashContent::~FlashContent() {
	close();
}

void FlashContent::initialize() {
	_scene = dynamic_cast<FlashScenePtr>(_renderer.getScene("flash"));
}

/** ファイルをオープンします */
bool FlashContent::open(const MediaItemPtr media, const int offset) {
	if (!_scene) return false;
	//Poco::ScopedLock<Poco::FastMutex> lock(_lock);

	//load the movie
	//_log.information(Poco::format("ready state before: %ld", _flash->ReadyState));
	MediaItemFile mif = media->files()[0];
	if (mif.file().find("http://") == 0) {
		_movie = mif.file();
	} else {
		//file = "file://" + Path(mif.file()).absolute(config().dataRoot).toString(Poco::Path::PATH_UNIX);
		_movie = Path(mif.file()).absolute(config().dataRoot).toString();
	}

	set("alpha", 1.0f);
	_duration = media->duration() * 60 / 1000;
	_current = 0;
	_mediaID = media->id();
	return true;
}


/**
 * 再生
 */
void FlashContent::play() {
	if (_scene) {
		_scene->loadMovie(_movie);
		_playing = true;
	}
}

/**
 * 停止
 */
void FlashContent::stop() {
	if (_scene) {
		_playing = false;
		_scene->loadMovie("");
	}
}

bool FlashContent::useFastStop() {
	return false;
}

/**
 * 再生中かどうか
 */
const bool FlashContent::playing() const {
	return _playing;
}

const bool FlashContent::finished() {
	if (_scene && _playing) {
	//	return _flash->IsPlaying() == VARIANT_FALSE;
		return _current >= _duration;
	}
	return false;
}

/** ファイルをクローズします */
void FlashContent::close() {
	stop();
	_mediaID.clear();
}

void FlashContent::process(const DWORD& frame) {
	if (_playing) {
		_current++;
	}
}

void FlashContent::draw(const DWORD& frame) {
	LPDIRECT3DTEXTURE9 buf = _scene->getTexture();
	if (buf) {
		float alpha = getF("alpha");
		DWORD col = ((DWORD)(0xff * alpha) << 24) | 0xffffff;
		_renderer.drawTexture(_x, _y, buf, 0, col, col, col, col);
	}
}

#endif
