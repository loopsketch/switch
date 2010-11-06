#include <Poco/DateTime.h>
#include <Poco/Timezone.h>
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>
#include <Poco/Util/XMLConfiguration.h>
#include <Poco/UnicodeConverter.h>

#include "MixContent.h"
#include "ImageContent.h"
#include "FFMovieContent.h"
#include "DSContent.h"
#include "TextContent.h"
#include "FlashContent.h"
#include "Utils.h"


MixContent::MixContent(Renderer& renderer, int splitType): Content(renderer, splitType), 
	_playing(false)
{
	initialize();
}

MixContent::~MixContent() {
	initialize();
	for (vector<ContentPtr>::const_iterator it = _contents.begin(); it != _contents.end(); it++) delete (*it);
}

void MixContent::initialize() {
	close();
}

/** ファイルをオープンします */
bool MixContent::open(const MediaItemPtr media, const int offset) {
	initialize();

	vector<string> blocks; // 0-0-1080-600|0-600-1080-720|0-1320-1080-600
	svvitch::split(Poco::trim(media->getProperty("blocks")), '|', blocks);

	int i = 0;
	for (vector<MediaItemFile>::const_iterator it = media->files().begin(); it != media->files().end(); it++) {
		MediaItemFile mif = *it;
		double x = 0;
		double y = 0;
		double w = _w;
		double h = _h;
		if (blocks.size() > i) {
			vector<string> nums; // 0-0-1080-600|0-600-1080-720|0-1320-1080-600
			svvitch::split(blocks.at(i), '-', nums);
			if (nums.size() == 4) {
				Poco::NumberParser::tryParseFloat(Poco::trim(nums[0]), x);
				Poco::NumberParser::tryParseFloat(Poco::trim(nums[1]), y);
				Poco::NumberParser::tryParseFloat(Poco::trim(nums[2]), w);
				Poco::NumberParser::tryParseFloat(Poco::trim(nums[3]), h);
			} else {
				_log.warning(Poco::format("failed blocks setting: %s", media->getProperty("blocks")));
			}
		} else {
			_log.warning(Poco::format("failed blocks setting: %d > %s", i, media->getProperty("blocks")));
		}
		ContentPtr c = NULL;
		switch (mif.type()) {
		case MediaTypeImage:
			c = new ImageContent(_renderer, config().splitType);
			c->set("aspect-mode", "lefttop");
			break;
		case MediaTypeMovie:
			for (vector<string>::iterator it = config().movieEngines.begin(); it < config().movieEngines.end(); it++) {
				string engine = Poco::toLower(*it);
				if (engine == "ffmpeg") {
					c = new FFMovieContent(_renderer, config().splitType);
				} else if (engine == "directshow") {
					c = new DSContent(_renderer, config().splitType);
				} else {
					_log.warning(Poco::format("failed not found movie engine: %s", engine));
				}
			}
			break;
		case MediaTypeFlash:
			c = new FlashContent(_renderer, config().splitType, x, y, w, h);
			break;
		}
		if (c) {
			if (c->open(media, i)) {
				c->setPosition(x, y);
				c->setBounds(w, h);
				c->set("alpha", 1.0f);
				_contents.push_back(c);
			} else {
				SAFE_DELETE(c);
			}
		}
		i++;
	}

	_duration = media->duration() * 60 / 1000;
	_current = 0;
	_mediaID = media->id();
	return true;
}


/**
 * 再生
 */
void MixContent::play() {
	for (vector<ContentPtr>::const_iterator it = _contents.begin(); it != _contents.end(); it++) (*it)->play();
}

/**
 * 停止
 */
void MixContent::stop() {
	for (vector<ContentPtr>::const_iterator it = _contents.begin(); it != _contents.end(); it++) (*it)->stop();
}

bool MixContent::useFastStop() {
	return true;
}

/**
 * 再生中かどうか
 */
const bool MixContent::playing() const {
	bool playing = false;
	for (vector<ContentPtr>::const_iterator it = _contents.begin(); it != _contents.end(); it++) {
		playing |= (*it)->playing();
	}
	return playing;
}

const bool MixContent::finished() {
	if (!_mediaID.empty()) {
		bool finished = true;
		for (vector<ContentPtr>::const_iterator it = _contents.begin(); it != _contents.end(); it++) {
			finished &= (*it)->finished();
		}
		return finished;
	}
	return true;
}

/** ファイルをクローズします */
void MixContent::close() {
	for (vector<ContentPtr>::const_iterator it = _contents.begin(); it != _contents.end(); it++) (*it)->close();
	_mediaID.clear();
}

void MixContent::process(const DWORD& frame) {
	string time, current, remain, fps, status;
	for (vector<ContentPtr>::const_iterator it = _contents.begin(); it != _contents.end(); it++) {
		ContentPtr c = *it;
		c->process(frame);
		if (!c->finished()) {
			string s = c->get("time"); if (!s.empty()) time = s;
			s = c->get("time_current"); if (!s.empty()) current = s;
			s = c->get("time_remain"); if (!s.empty()) remain = s;
			s = c->get("time_fps"); if (!s.empty()) fps = s;
			s = c->get("status"); if (!s.empty()) status = s;
			set("time", time);
			set("time_current", current);
			set("time_remain", remain);
			set("time_fps", fps);
			set("status", status);
		}
	}
}

void MixContent::draw(const DWORD& frame) {
	for (vector<ContentPtr>::const_iterator it = _contents.begin(); it != _contents.end(); it++) (*it)->draw(frame);
}
