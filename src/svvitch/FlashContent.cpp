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
	_log.information("flash initialized");
}

/** ファイルをオープンします */
bool FlashContent::open(const MediaItemPtr media, const int offset) {
	//Poco::ScopedLock<Poco::FastMutex> lock(_lock);

	//load the movie
	//_log.information(Poco::format("ready state before: %ld", _flash->ReadyState));
	MediaItemFile mif = media->files()[0];
	string file;
	if (mif.file().find("http://") == 0) {
		file = mif.file();
	} else {
		//file = "file://" + Path(mif.file()).absolute(config().dataRoot).toString(Poco::Path::PATH_UNIX);
		file = Path(mif.file()).absolute(config().dataRoot).toString();
	}
	_log.information(Poco::format("movie: %s", file));
	if (!_scene->loadMovie(file)) {
		_log.warning(Poco::format("failed movie: %s", file));
		return false;
	}
	//string sjis;
	//svvitch::utf8_sjis(file, sjis);
	//wstring wfile;
	//Poco::UnicodeConverter::toUTF16(file, wfile);

	set("alpha", 1.0f);
	//_duration = _flash->GetTotalFrames();
	//_flash->StopPlay();
	_duration = media->duration() * 60 / 1000;
	_current = 0;
	_mediaID = media->id();
	return true;
}


/**
 * 再生
 */
void FlashContent::play() {
	_playing = true;
}

/**
 * 停止
 */
void FlashContent::stop() {
	_playing = false;
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
	//if (_flash && _playing) {
	//	return _flash->IsPlaying() == VARIANT_FALSE;
	//}
	return false;
	//return _current >= _duration;
}

/** ファイルをクローズします */
void FlashContent::close() {
	stop();
	_mediaID.clear();
}

void FlashContent::process(const DWORD& frame) {
}

void FlashContent::draw(const DWORD& frame) {
}

#endif
