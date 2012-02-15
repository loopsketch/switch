
#include "TelopScene.h"
#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeParser.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/Timezone.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/XML/XMLWriter.h>
#include <Poco/SAX/InputSource.h>
#include <Poco/UnicodeConverter.h>
#include <sstream>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include "Utils.h"


TelopScene::TelopScene(Renderer& renderer): Scene(renderer) {
}

TelopScene::~TelopScene() {
	_log.information("*release TelopScene");
	_worker = NULL;
	_thread.join();
}

bool TelopScene::initialize() {
	_log.information("*initialize TelopScene");
	_remoteURL = "http://rss.asahi.com/rss/asahi/newsheadlines.rdf";
	_validMinutes = 120;
	_speed = -1;
	_space = 200;
	string s;
	//Poco::UnicodeConverter::toUTF8(L"YD,T0,T1,T2,T3,T4,T5,T6,JW01", s);
	//Poco::UnicodeConverter::toUTF8(L"社会,スポーツ,デジタル", s);
	//svvitch::split(s, ',', _categories); // ent,int,pol,soc,eco,spo
	_creating = false;

	_worker = this;
	_thread.start(*_worker);
	return true;
}

void TelopScene::run() {
	queue<string> texts;
	int i = -1;
	while (_worker) {
		Poco::LocalDateTime now;
		map<string, vector<string>> sources = readXML(now);

		for (int t = 0; _worker && t < 300; ++t) {
			if (texts.empty()) {
				for (map<string, vector<string>>::iterator m = sources.begin(); m != sources.end(); ++m) {
					for (vector<string>::iterator it = m->second.begin(); it != m->second.end(); ++it) {
						texts.push(*it);
					}
					_log.information(Poco::format("add source[%s] %u", m->first, m->second.size()));
				}
			}

			if (!texts.empty() && _prepared.empty() && _creating) {
				_creating = false;
				string text = texts.front();
				texts.pop();
				if (!text.empty()) {
					LPDIRECT3DTEXTURE9 texture = _renderer.createTexturedText(L"", 24, 0xccffffff, 0x9999ccff, 2, 0xcc000000, 0, 0xff000000, text);
					if (texture) {
						D3DSURFACE_DESC desc;
						HRESULT hr = texture->GetLevelDesc(0, &desc);
						Telop telop;
						telop.w = desc.Width;
						telop.h = desc.Height;
						telop.x = config().stageRect.right;
						telop.y = config().stageRect.bottom - telop.h;
						telop.texture = texture;
						Poco::ScopedLock<Poco::FastMutex> lock(_lock);
						_prepared.push(telop);
					}
				}
			} else {
				Poco::Thread::sleep(900);
			}
			if (!_deletes.empty()) {
				Telop t = _deletes.front();
				_deletes.pop();
				t.texture->Release();
			}
			Poco::Thread::sleep(100);
		}
	}

	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	for (vector<Telop>::iterator it = _telops.begin(); it != _telops.end(); ++it) {
		Telop& t = *it;
		t.texture->Release();
	}
	while (!_prepared.empty()) {
		Telop t = _prepared.front();
		_prepared.pop();
		t.texture->Release();
	}
	while (!_deletes.empty()) {
		Telop t = _deletes.front();
		_deletes.pop();
		t.texture->Release();
	}
}

map<string, vector<string>> TelopScene::readXML(const Poco::LocalDateTime& now) {
	// rss1.0
	_log.information(Poco::format("check remote[%s]: %s", Poco::DateTimeFormatter::format(now, Poco::DateTimeFormat::SORTABLE_FORMAT), _remoteURL));
	map<string, vector<string>> sources;
	Poco::XML::DOMParser parser;
	try {
		Poco::XML::Document* doc = parser.parse(_remoteURL);
		if (doc) {
			Poco::XML::NodeList* list = doc->getElementsByTagName("item");
			for (int i = 0; i < list->length(); i++) {
				Poco::XML::Element* e = (Poco::XML::Element*)list->item(i);
				Poco::XML::Element* date = e->getChildElement("dc:date");
				int tzd = 0;
				Poco::DateTime dt;
				if (date) {
					dt = Poco::DateTimeParser::parse(Poco::DateTimeFormat::ISO8601_FORMAT, date->innerText(), tzd);
					if (now.day() != dt.day()) continue;
					int d = (now.timestamp() - dt.timestamp()) / (60 * Poco::Timestamp::resolution());
					if (d > _validMinutes) continue;
					//_log.information(Poco::format("date: %s %d", date->innerText(), d));
				}
				vector<string> categories;
				Poco::XML::Element* category = e->getChildElement("dc:subject");
				if (category) svvitch::split(category->innerText(), ',', categories);
				if (categories.empty()) continue;
				for (vector<string>::iterator it = categories.begin(); it != categories.end(); ++it) {
					//vector<string>::iterator f = _categories.find(key);
				}
				Poco::XML::Element* title = e->getChildElement("title");
				if (title) {
					for (vector<string>::iterator it = categories.begin(); it != categories.end(); ++it) {
						const string& key = *it;
						map<string, vector<string>>::iterator m = sources.find(key);
						if (m == sources.end()) {
							vector<string> vec;
							sources[key] = vec;
						}
						sources[key].push_back(title->innerText());
					}
				}
			}
			list->release();
			doc->release();

			int count = 0;
			for (map<string, vector<string>>::iterator m = sources.begin(); m != sources.end(); ++m) {
				//_log.information(Poco::format("category: %s",m->first));
				//for (vector<string>::iterator it = m->second.begin(); it != m->second.end(); ++it) {
				//	_log.information(Poco::format("text[%s]",*it));
				//}
				count += m->second.size();
			}
			_log.information(Poco::format("remote reading %ucategories, %dtexts", sources.size(), count));
		} else {
			_log.warning("failed not parse remote XML");
		}
	} catch (Poco::IOException& ex) {
		_log.warning(Poco::format("failed not read XML: %s", ex.displayText()));
	} catch (...) {
		_log.warning(Poco::format("failed exception in readMXL(): %s", _remoteURL));
	}
	return sources;
}

map<string, vector<string>> TelopScene::_readXML(const Poco::LocalDateTime& now) {
	_log.information(Poco::format("check remote[%s]: %s", Poco::DateTimeFormatter::format(now, Poco::DateTimeFormat::SORTABLE_FORMAT), _remoteURL));
	map<string, vector<string>> sources;
	Poco::XML::DOMParser parser;
	try {
		Poco::XML::Document* doc = parser.parse(_remoteURL);
		if (doc) {
			Poco::XML::NodeList* list = doc->getElementsByTagName("item");
			for (int i = 0; i < list->length(); i++) {
				Poco::XML::Element* e = (Poco::XML::Element*)list->item(i);
				Poco::XML::Element* date = e->getChildElement("date");
				int tzd = 0;
				Poco::DateTime dt;
				if (date) {
					dt = Poco::DateTimeParser::parse("%Y/%m/%d %H:%M:%S", date->innerText(), tzd);
					if (now.day() != dt.day()) continue;
					int d = (now.timestamp() - dt.timestamp()) / (60 * Poco::Timestamp::resolution());
					if (d > _validMinutes) continue;
					//_log.information(Poco::format("date: %s %d", date->innerText(), d));
				}
				Poco::XML::Element* command = e->getChildElement("command");
				if (command && command->innerText() != "9") {
					vector<string> categories;
					Poco::XML::Element* category = e->getChildElement("category");
					if (category) svvitch::split(category->innerText(), ',', categories);
					if (categories.empty()) continue;
					Poco::XML::Element* title = e->getChildElement("title");
					if (title) {
						for (vector<string>::iterator it = categories.begin(); it != categories.end(); ++it) {
							const string& key = *it;
							map<string, vector<string>>::iterator m = sources.find(key);
							if (m == sources.end()) {
								vector<string> vec;
								sources[key] = vec;
							}
							sources[key].push_back(title->innerText());
						}
					}
					//Poco::XML::Element* caption = e->getChildElement("caption");
					//if (caption) _log.information(Poco::format("caption: %s", caption->innerText()));
				}
			}
			list->release();
			doc->release();

			int count = 0;
			for (map<string, vector<string>>::iterator m = sources.begin(); m != sources.end(); ++m) {
				//_log.information(Poco::format("category: %s",m->first));
				//for (vector<string>::iterator it = m->second.begin(); it != m->second.end(); ++it) {
				//	_log.information(Poco::format("text[%s]",*it));
				//}
				count += m->second.size();
			}
			_log.information(Poco::format("remote reading %ucategories, %dtexts", sources.size(), count));
		} else {
			_log.warning("failed not parse remote XML");
		}
	} catch (Poco::IOException& ex) {
		_log.warning(Poco::format("failed not read XML: %s", ex.displayText()));
	} catch (...) {
		_log.warning(Poco::format("failed exception in readMXL(): %s", _remoteURL));
	}
	return sources;
}


void TelopScene::process() {
	if (_telops.empty()) {
		_creating = true;
	} else {
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			bool creating = true;
			for (vector<Telop>::iterator it = _telops.begin(); it != _telops.end(); ) {
				Telop& t = *it;
				creating &= t.x < config().stageRect.right - t.w;
				if (t.x < -t.w) {
					_deletes.push(t);
					it = _telops.erase(it);
					continue;
				}
				t.x += _speed;
				++it;
			}
			_creating = creating;
		}
	}
	if (!_prepared.empty()) {
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			bool add = true;
			for (vector<Telop>::iterator it = _telops.begin(); it != _telops.end(); ++it) {
				Telop& t = *it;
				add &= t.x < config().stageRect.right - t.w - _space;
			}
			if (add) {
				_telops.push_back(_prepared.front());
				_prepared.pop();
			}
		}
	}
}

void TelopScene::draw1() {
	DWORD col = 0xffffffff;
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		for (vector<Telop>::iterator it = _telops.begin(); it != _telops.end(); ++it) {
			Telop& t = *it;
			_renderer.drawTexture(t.x, t.y, t.texture, 0, col, col, col, col);
		}
	}
}

void TelopScene::draw2() {
}
