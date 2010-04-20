#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/XML/XMLWriter.h>
#include <Poco/DateTime.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeParser.h>
#include <Poco/Timezone.h>
#include <Poco/Exception.h>
#include <Poco/FileStream.h>
#include <Poco/NumberParser.h>
#include <Poco/format.h>
#include <Poco/hash.h>
#include <Poco/string.h>
#include <Poco/RegularExpression.h>
#include <Poco/UnicodeConverter.h>

#include "Workspace.h"
#include "Utils.h"

using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::NodeList;


Workspace::Workspace(Path file): _log(Poco::Logger::get("")), _file(file) {
}

Workspace::~Workspace() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	release();
}


void Workspace::release() {
	for (vector<SchedulePtr>::iterator it = _schedule.begin(); it != _schedule.end(); it++) {
		SAFE_DELETE(*it);
	}
	for (vector<PlayListPtr>::iterator it = _playlist.begin(); it != _playlist.end(); it++) {
		PlayListPtr playlist = *it;
		// _renderer.removeCachedTexture(playlist->name());
		SAFE_DELETE(playlist);
	}
	_playlistMap.clear();
	_playlist.clear();

	for (vector<MediaItemPtr>::iterator it = _media.begin(); it != _media.end(); it++) {
		MediaItemPtr media = *it;
//		_renderer.removeCachedTexture(media->id());
		SAFE_DELETE(media);
	}
	_mediaMap.clear();
	_media.clear();
	_log.information("release workspace");
}

const Path& Workspace::file() const {
	return _file;
}

bool Workspace::checkUpdate() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	string signature = svvitch::md5(_file.toString());
	if (_signature != signature) {
		_log.information(Poco::format("detect update workspace: %s", _signature));
		return true;
	}
	return false;
}

bool Workspace::parse() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	try {
		string signature = svvitch::md5(_file);
		Poco::XML::DOMParser parser;
		Document* doc = parser.parse(_file.toString());
		if (doc) {
			_log.information(Poco::format("update/parse workspace file: %s", _file.toString()));
			release();
			NodeList* nodes = doc->documentElement()->getElementsByTagName("medialist");
			if (nodes) {
				_media.clear();
				for (int i = 0; i < nodes->length(); i++) {
					Element* e = (Element*)nodes->item(i);
					NodeList* items = e->getElementsByTagName("item");
					for (int j = 0; j < items->length(); j++) {
						e = (Element*)items->item(j);
						string type = Poco::toLower(e->getAttribute("type"));
						string name = e->getAttribute("name");
						string id = e->getAttribute("id");
						string d = e->getAttribute("duration");
						bool stanby = Poco::toLower(e->getAttribute("stanby")) == "true";
						int duration = 0;
						if (!d.empty()) {
							duration = Poco::NumberParser::parse(d);
						}
						string parameters = e->getAttribute("params");
						vector<MediaItemFile> files;
						NodeList* movies = e->getElementsByTagName("*");
						for (int k = 0; k < movies->length(); k++) {
							e = (Element*)movies->item(k);
							string file = e->innerText();
							if (file.find("switch-data:/") == 0) {
								file = Path(config().dataRoot, file.substr(13)).toString();
							}
							string params;
							if (file.find("?") != string::npos) {
								params = file.substr(file.find("?") + 1);
								file = file.substr(0, file.find("?"));
							}
							if (e->tagName() == "movie") {
								files.push_back(MediaItemFile(MediaTypeMovie, file, params));
							} else if (e->tagName() == "image") {
								files.push_back(MediaItemFile(MediaTypeImage, file, params));
							} else if (e->tagName() == "text") {
								files.push_back(MediaItemFile(MediaTypeText, file, params));
							}
						}
						movies->release();

						// MediaTypeImage
						MediaType typeCode = MediaTypeUnknown;
						if (type == "movie") {
							typeCode = MediaTypeMovie;
						} else if (type == "image") {
							typeCode = MediaTypeImage;
						} else  if (type == "cv") {
							typeCode = MediaTypeCv;
						} else  if (type == "cvcap") {
							typeCode = MediaTypeCvCap;
						} else  if (type == "game") {
							typeCode = MediaTypeGame;
						}
						Poco::HashMap<string, MediaItemPtr>::Iterator find = _mediaMap.find(id);
						if (find != _mediaMap.end()) {
							_log.warning(Poco::format("already registed media, delete old item: %s", id));
							for (vector<MediaItemPtr>::iterator it = _media.begin(); it != _media.end(); it++) {
								MediaItemPtr media = *it;
								if (id == media->id()) {
									_media.erase(it);
									break;
								}
							}
							SAFE_DELETE(find->second);
							_mediaMap.erase(find);
						}
						MediaItemPtr media  = new MediaItem(typeCode, id, name, duration, stanby, files);
						_mediaMap[id] = media;
						_media.push_back(media);
//						LPDIRECT3DTEXTURE9 texture = _renderer.createTexturedText(L"", 18, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, _media[id]->name());
//						_renderer.addCachedTexture(id, texture);
//						_log.debug(Poco::format("media: <%s> %s %d", id, name, duration));
					}
					items->release();
				}
				nodes->release();
			}
			_log.information(Poco::format("media item: %?u", _media.size()));

			nodes = doc->documentElement()->getElementsByTagName("playlists");
			if (nodes) {
				for (int i = 0; i < nodes->length(); i++) {
					Element* e = (Element*)nodes->item(i);
					NodeList* nodesPlaylist = e->getElementsByTagName("playlist");
					for (int j = 0; j < nodesPlaylist->length(); j++) {
						e = (Element*)nodesPlaylist->item(j);
						string name = e->getAttribute("name");
						string id = e->getAttribute("id");
						if (id.empty()) {
							// TODO playlist.idŽw’è‚ª–³‚¢Žž‚Ìˆ—
							id = Poco::format("g%04d", j);
//							_log.information(Poco::format("auto group id: %s", id));
						}
						string text = e->getAttribute("text");
						PlayListPtr playlist = new PlayList(id, name, text);
//						LPDIRECT3DTEXTURE9 texture = _renderer.createTexturedText(L"", 18, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, name);
//						_renderer.addCachedTexture(name, texture);
						NodeList* items = e->getElementsByTagName("item");
						for (int k = 0; k < items->length(); k++) {
							e = (Element*)items->item(k);
							string id = e->innerText();
							string next = e->getAttribute("next");
							string transition = e->getAttribute("transition");
							Poco::HashMap<string, MediaItemPtr>::Iterator it = _mediaMap.find(id);
							if (it != _mediaMap.end()) {
								playlist->add(new PlayListItem(it->second, next, transition));
							} else {
								_log.warning(Poco::format("not found item[id:%s] in playlist[name:%s]", id, name));
							}
						}
						_playlistMap[id] = playlist;
						_playlist.push_back(playlist);
//						_log.debug(Poco::format("playlist: %s x%d", playlist->name(), playlist->itemCount()));
						items->release();
					}
					nodesPlaylist->release();
				}
				nodes->release();
			}
			_log.information(Poco::format("playlist: %?u", _playlist.size()));

			nodes = doc->documentElement()->getElementsByTagName("schedule");
			if (nodes) {
				Poco::RegularExpression re("[\\s:/]+");
				for (int i = 0; i < nodes->length(); i++) {
					Element* schedule = (Element*)nodes->item(i);
					NodeList* items = schedule->getElementsByTagName("item");
					for (int j = 0; j < items->length(); j++) {
						Element* e = (Element*)items->item(j);
						string id = e->getAttribute("id");
						string t = e->getAttribute("time");
						string command = e->innerText();
						int pos = 0;
						Poco::RegularExpression::Match match;
						vector<int> time;
						while (re.match(t, pos, match) > 0) {
							string s = t.substr(pos, match.offset - pos);
							if (s == "*") {
								time.push_back(-1);
							} else {
								time.push_back(Poco::NumberParser::parse(s));
							}
							pos = (match.offset + match.length);
						}
						string s = t.substr(pos);
						if (s == "*") {
							time.push_back(-1);
						} else {
							time.push_back(Poco::NumberParser::parse(s));
						}
						if (time.size() == 6) {
							SchedulePtr schedule = new Schedule(id, time[0], time[1], time[2], time[3], time[4], 0, time[5], command);
							_schedule.push_back(schedule);
						}
					}
					items->release();
				}
				nodes->release();
			}
			nodes = doc->documentElement()->getElementsByTagName("deletes");
			if (nodes) {
				int tzd = Poco::Timezone::tzd();
				//Poco::LocalDateTime now;
				Poco::DateTime now;
				now.makeLocal(tzd);
				Poco::Timespan span(7, 0, 0, 0, 0); // 7days
				Poco::DateTime validateTime = now - span;
				bool update = false;
				for (int i = 0; i < nodes->length(); i++) {
					Element* deletes = (Element*)nodes->item(i);
					NodeList* files = deletes->getElementsByTagName("file");
					for (int j = 0; j < files->length(); j++) {
						Element* e = (Element*)files->item(j);
						string path = e->innerText();
						if (path.find("switch-data:/") == 0) {
							path = Path(config().dataRoot, path.substr(13)).toString();
						}
						try {
							File file(path);
							if (file.exists()) {
								file.remove();
								_log.information(Poco::format("file delete: %s", file.path()));
							}
						} catch (Poco::FileException ex) {
							_log.warning(ex.displayText());
						}

						string date = e->getAttribute("date");
						Poco::DateTime deleteDate;
						Poco::DateTimeParser::tryParse(Poco::DateTimeFormat::ISO8601_FORMAT, date, deleteDate, tzd);
						if (validateTime > deleteDate) {
							_log.information(Poco::format("purge element: %s", e->innerText()));
							deletes->removeChild(e);
							update = true;
						}
					}
					files->release();
				}
				nodes->release();
				if (update) {
					try {
						Poco::FileOutputStream os(_file.toString());
						if (os.good()) {
							Poco::XML::DOMWriter writer;
							writer.setNewLine("\r\n");
							writer.setOptions(Poco::XML::XMLWriter::WRITE_XML_DECLARATION | Poco::XML::XMLWriter::PRETTY_PRINT);
							writer.writeNode(os, doc);
							_log.information(Poco::format("update: %s", _file.toString()));
						}
					} catch (Poco::Exception& ex) {
						_log.warning(ex.displayText());
					}
					signature = svvitch::md5(_file);
				}
			}
			doc->release();
			_signature = signature;
			return true;
		} else {
			_log.warning(Poco::format("failed parse: %s", _file));
		}
	} catch (Poco::Exception& ex) {
		_log.warning(ex.displayText());
	}
	return false;
}

const int Workspace::getMediaCount() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	return _media.size();
}

const MediaItemPtr Workspace::getMedia(int i) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (i >= 0 && i < _media.size()) {
		return _media[i];
	}
	return NULL;
}

const MediaItemPtr Workspace::getMedia(string id) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	Poco::HashMap<string, MediaItemPtr>::Iterator it = _mediaMap.find(id);
	if (it != _mediaMap.end()) {
		return it->second;
	}
	return NULL;
}


const int Workspace::getPlaylistCount() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	return _playlist.size();
}

const PlayListPtr Workspace::getPlaylist(int i) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (i >= 0 && i < _playlist.size()) {
		return _playlist[i];
	}
	return NULL;
}

const PlayListPtr Workspace::getPlaylist(string id) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	Poco::HashMap<string, PlayListPtr>::Iterator it = _playlistMap.find(id);
	if (it != _playlistMap.end()) {
		return it->second;
	}
	return NULL;
}

const int Workspace::getScheduleCount() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	return _schedule.size();
}

const SchedulePtr Workspace::getSchedule(int i) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (i >= 0 && i < _schedule.size()) {
		return _schedule[i];
	}
	return NULL;
}

