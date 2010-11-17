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
#include <Poco/format.h>
#include <Poco/hash.h>
#include <Poco/string.h>
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
				_existsFiles.clear();
				for (int i = 0; i < nodes->length(); i++) {
					Element* e = (Element*)nodes->item(i);
					NodeList* items = e->getElementsByTagName("item");
					for (int j = 0; j < items->length(); j++) {
						e = (Element*)items->item(j);
						string type = Poco::toLower(e->getAttribute("type"));
						string name = e->getAttribute("name");
						string id = e->getAttribute("id");
						string s = e->getAttribute("start");
						int start = 0;
						if (!s.empty()) {
							start = Poco::NumberParser::parse(s);
						}
						string d = e->getAttribute("duration");
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
							} else if (e->tagName() == "flash") {
								files.push_back(MediaItemFile(MediaTypeFlash, file, params));
							} else {
								files.push_back(MediaItemFile(MediaTypeUnknown, file, params));
							}
						}
						movies->release();

						MediaType typeCode = MediaTypeUnknown;
						if (type == "mix") {
							typeCode = MediaTypeMix;
						} else if (type == "movie") {
							typeCode = MediaTypeMovie;
						} else if (type == "image") {
							typeCode = MediaTypeImage;
						} else  if (type == "flash") {
							typeCode = MediaTypeFlash;
						} else  if (type == "browser") {
							typeCode = MediaTypeBrowser;
						} else  if (type == "cv") {
							typeCode = MediaTypeCv;
						} else  if (type == "cvcap") {
							typeCode = MediaTypeCvCap;
						} else  if (type == "game") {
							typeCode = MediaTypeGame;
						} else  if (type == "game2") {
							typeCode = MediaTypeGame2;
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
						MediaItemPtr media  = new MediaItem(typeCode, id, name, start, duration, parameters, files);
						_mediaMap[id] = media;
						_media.push_back(media);
						switch (media->type()) {
						case MediaTypeMix:
						case MediaTypeMovie:
						case MediaTypeImage:
						case MediaTypeText:
						case MediaTypeFlash:
							for (vector<MediaItemFile>::const_iterator it = files.begin(); it != files.end(); it++) {
								if (!((*it).file().empty()) && (*it).file().find("http") == string::npos) {
									try {
										File f = File((*it).file());
										if (f.exists()) _existsFiles.push_back(f.path());
									} catch (Poco::FileException& ex) {
									}
								}
							}
						}
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
				for (int i = 0; i < nodes->length(); i++) {
					Element* schedule = (Element*)nodes->item(i);
					NodeList* items = schedule->getElementsByTagName("item");
					for (int j = 0; j < items->length(); j++) {
						Element* e = (Element*)items->item(j);
						string id = e->getAttribute("id");
						string t = e->getAttribute("time");
						string command = e->innerText();
						vector<int> time = svvitch::parseTimes(t);
						if (time.size() == 7) {
							SchedulePtr schedule = new Schedule(id, time[0], time[1], time[2], time[3], time[4], time[5], time[6], command);
							_schedule.push_back(schedule);
						} else if (time.size() == 6) {
							// spec of old
							SchedulePtr schedule = new Schedule(id, time[0], time[1], time[2], time[3], time[4], 0, time[5], command);
							_schedule.push_back(schedule);
						}
					}
					items->release();
				}
				nodes->release();
			}

			nodes = doc->documentElement()->getElementsByTagName("fonts");
			if (nodes) {
				for (int i = 0; i < nodes->length(); i++) {
					Element* fonts = (Element*)nodes->item(i);
					NodeList* files = fonts->getElementsByTagName("file");
					for (int j = 0; j < files->length(); j++) {
						Element* e = (Element*)files->item(j);
						string path = e->innerText();
						if (path.find("switch-data:/") == 0) {
							Path p(config().dataRoot, path.substr(13));
							path = p.parse(p.toString()).toString();
						}
						_fonts.push_back(path);
					}
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

const vector<string> Workspace::getFonts() {
	return _fonts;
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

const vector<string> Workspace::existsFiles() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	return _existsFiles;
}

const string Workspace::signature() const {
	return _signature;
}
