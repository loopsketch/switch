#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/Exception.h>
#include <Poco/NumberParser.h>
#include <Poco/format.h>
#include <Poco/string.h>
#include <Poco/UnicodeConverter.h>

#include "Workspace.h"
#include "Image.h"
#include "Movie.h"
#include "Text.h"
#include "CvContent.h"
#include "CaptureContent.h"
#include "DSContent.h"

using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::NodeList;


Workspace::Workspace(Renderer& renderer): _log(Poco::Logger::get("")), _renderer(renderer) {
}

Workspace::~Workspace() {
	initialize();
}

void Workspace::initialize() {
	for (vector<PlayListPtr>::iterator it = _playlists.begin(); it != _playlists.end(); it++) {
		PlayListPtr playlist = *it;
		_renderer.removeCachedTexture(playlist->name());
		SAFE_DELETE(playlist);
	}
	_playlists.clear();
	for (Poco::HashMap<string, MediaItemPtr>::Iterator it = _media.begin(); it != _media.end(); it++) {
		MediaItemPtr media = it->second;
		_renderer.removeCachedTexture(media->id());
		SAFE_DELETE(media);
	}
	_media.clear();
}


/** XML‚ðƒp[ƒX */
bool Workspace::parse(const string file) {
	initialize();
	try {
		Poco::XML::DOMParser parser;
		Document* doc = parser.parse(file);
		if (doc) {
			_log.information(Poco::format("parse workspace: %s", file));
			NodeList* nodes = doc->documentElement()->getElementsByTagName("medialist");
			if (nodes) {
				_log.information("parse media items");
				_media.clear();
				for (int i = 0; i < nodes->length(); i++) {
					Element* e = (Element*)nodes->item(i);
					NodeList* items = e->getElementsByTagName("item");
					for (int j = 0; j < items->length(); j++) {
						e = (Element*)items->item(j);
						string type = Poco::toLower(e->getAttribute("type"));
						string id = e->getAttribute("id");
						string name = e->getAttribute("name");
						string d = e->getAttribute("duration");
						string parameters = e->getAttribute("params");
						int duration = 0;
						if (d.length() > 0) {
							duration = Poco::NumberParser::parse(e->getAttribute("duration"));
						}
						vector<MediaItemFilePtr> files;
						NodeList* movies = e->getElementsByTagName("*");
						for (int k = 0; k < movies->length(); k++) {
							e = (Element*)movies->item(k);
							string file = e->innerText();
							string params;
							if (file.find("?") != string::npos) {
								params = file.substr(file.find("?") + 1);
								file = file.substr(0, file.find("?"));
							}
							if (e->tagName() == "movie") {
								files.push_back(new MediaItemFile(MediaTypeMovie, file, params));
							} else if (e->tagName() == "image") {
								files.push_back(new MediaItemFile(MediaTypeImage, file, params));
							} else if (e->tagName() == "text") {
								files.push_back(new MediaItemFile(MediaTypeText, file, params));
							}
						}
						movies->release();

						// MediaTypeImage
						MediaType typeCode = MediaTypeMovie;
						if (type == "movie") {
						} else if (type == "image") {
							typeCode = MediaTypeImage;
						} else  if (type == "cv") {
							typeCode = MediaTypeCv;
						} else  if (type == "cvcap") {
							typeCode = MediaTypeCvCap;
						}
						if (_media.find(id) != _media.end()) {
							_log.warning(Poco::format("already registed media: %s", id));
							SAFE_DELETE(_media[id]);
							_media.erase(id);
						}
						_media[id] = new MediaItem(typeCode, id, name, duration, files);
						LPDIRECT3DTEXTURE9 texture = _renderer.createTexturedText(L"", 18, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, _media[id]->name());
						_renderer.addCachedTexture(id, texture);
						_log.debug(Poco::format("media: <%s> %s %d", id, name, duration));
					}
					items->release();
				}
				nodes->release();
			}

			nodes = doc->documentElement()->getElementsByTagName("playlists");
			if (nodes) {
				_log.information("parse playlists");
				_playlists.clear();
				for (int i = 0; i < nodes->length(); i++) {
					Element* e = (Element*)nodes->item(i);
					NodeList* nodesPlaylist = e->getElementsByTagName("playlist");
					for (int j = 0; j < nodesPlaylist->length(); j++) {
						e = (Element*)nodesPlaylist->item(j);
						string name = e->getAttribute("name");
						PlayListPtr playlist = new PlayList(name);
						LPDIRECT3DTEXTURE9 texture = _renderer.createTexturedText(L"", 18, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, name);
//						LPDIRECT3DTEXTURE9 texture = renderer.createTexturedText(L"", 18, 0xff99ffcc, 0xff99ccff, 4, 0xccffffff, 2, 0xff000000, name);
						_renderer.addCachedTexture(name, texture);
						NodeList* items = e->getElementsByTagName("item");
						for (int k = 0; k < items->length(); k++) {
							e = (Element*)items->item(k);
							string id = e->innerText();
							string next = e->getAttribute("next");
							string transition = e->getAttribute("transition");
							Poco::HashMap<string, MediaItemPtr>::Iterator it = _media.find(id);
							if (it != _media.end()) {
								playlist->add(new PlayListItem(it->second, next, transition));
							} else {
								_log.warning(Poco::format("not found item[id:%s] in playlist[name:%s]", id, name));
							}
						}
						_playlists.push_back(playlist);
						_log.debug(Poco::format("playlist: %s x%d", playlist->name(), playlist->itemCount()));
						items->release();
					}
					nodesPlaylist->release();
				}
				nodes->release();
			}
			doc->release();
			return true;
		} else {
			_log.warning(Poco::format("failed parse: %s", file));
		}
	} catch (Poco::Exception& ex) {
		_log.warning(ex.displayText());
	}
	return false;
}

void Workspace::release() {
}

const int Workspace::getPlayListCount() const {
	return _playlists.size();
}

const PlayListPtr Workspace::getPlayList(int i) const {
	try {
		return _playlists.at(i);
	} catch (std::out_of_range ex) {
	}
	return NULL;
}

PlayListItemPtr Workspace::prepareMedia(ContainerPtr container, PlayListPtr playlist, const int itemIndex) {
	if (playlist && playlist->itemCount() > 0) {
		int i = itemIndex % playlist->itemCount();
		PlayListItemPtr item = playlist->items()[i];
		MediaItemPtr media = item->media();
		if (media) {
			ConfigurationPtr conf = _renderer.config();
			container->initialize();
			switch (media->type()) {
				case MediaTypeImage:
					{
						ImagePtr image = new Image(_renderer);
						image->open(media);
						container->add(image);
					}
					break;

				case MediaTypeMovie:
					{
						MoviePtr movie = new Movie(_renderer);
//						DSContentPtr movie = new DSContent(_renderer);
						if (movie->open(media)) {
							movie->setPosition(conf->stageRect.left, conf->stageRect.top);
							movie->setBounds(conf->stageRect.right, conf->stageRect.bottom);
							container->add(movie);
						}
					}
					break;

				case MediaTypeText:
					break;

				case MediaTypeCv:
					{
						CvContentPtr cv = new CvContent(_renderer);
						cv->open(media);
						container->add(cv);
					}
					break;

				case MediaTypeCvCap:
					{
						CaptureContentPtr cvcap = new CaptureContent(_renderer);
						cvcap->open(media);
						container->add(cvcap);
					}
					break;

				default:
					_log.warning("media type: unknown");
			}
			if (media->containsFileType(MediaTypeText)) {
				for (int j = 0; j < media->fileCount(); j++) {
					TextPtr text = new Text(_renderer);
					if (text->open(media, j)) {
						container->add(text);
					} else {
						SAFE_DELETE(text);
					}
				}
			}
			_log.information(Poco::format("next prepared: %s", media->name()));
		} else {
			_log.warning("failed prepare next media, no media item");
		}
		return item;
	} else {
		_log.warning("failed prepare next media, no item in current playlist");
	}
	return NULL;
}
