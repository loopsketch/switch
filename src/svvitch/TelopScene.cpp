
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
#include <Poco/Util/XMLConfiguration.h>
#include "Utils.h"


TelopScene::TelopScene(Renderer& renderer): Scene(renderer), _visible(true) {
}

TelopScene::~TelopScene() {
	_log.information("*release TelopScene");
	_worker = NULL;
	_thread.join();
}

bool TelopScene::initialize() {
	_log.information("*initialize TelopScene");
	_sourceURL = "http://rss.rssad.jp/rss/mainichi/flash.rss";
	_validMinutes = 120;
	_speed = -1;
	_space = 200;
	try {
		Poco::Util::XMLConfiguration* config = new Poco::Util::XMLConfiguration("telop-config.xml");
		if (config) {
			_sourceURL = config->getString("sourceURL", "http://rss.rssad.jp/rss/mainichi/flash.rss");
			_validMinutes = config->getInt("validMinutes", 120);
			_speed = config->getInt("speed", -1);
			_space = config->getInt("space", 200);

			config->release();
		}
	} catch (Poco::Exception& ex) {
		_log.warning(ex.displayText());
	}

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
		map<string, vector<string>> sources = readSource(now);

		for (int t = 0; _worker && t < 300; ++t) {
			if (texts.empty()) {
				string format;
				Poco::UnicodeConverter::toUTF8(L"%Y”N%nŒŽ%d“ú%HŽž”Å", format);
				string date = Poco::DateTimeFormatter::format(_date, format);
				texts.push("title," + Poco::format("<<%s %s>>", _title, date));
				map<string, string> status;
				for (map<string, vector<string>>::iterator m = sources.begin(); m != sources.end(); ++m) {
					status[m->first] = svvitch::formatJSONArray(m->second);
					for (vector<string>::iterator it = m->second.begin(); it != m->second.end(); ++it) {
						texts.push(m->first + "," + (*it));
					}
					_log.information(Poco::format("add source[%s] %u", m->first, m->second.size()));
				}
				setStatus("telop", svvitch::formatJSON(status));
			}

			if (!texts.empty() && _prepared.empty() && _creating) {
				_creating = false;
				string text = texts.front();
				texts.pop();
				if (!text.empty()) {
					vector<string> datas;
					svvitch::split(text, ',', datas, 2);
					LPDIRECT3DTEXTURE9 texture = _renderer.createTexturedText(L"", 24, 0xccffffff, 0xcc3399ff, 2, 0xcc000000, 0, 0xff000000, datas[1]);
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

map<string, vector<string>> TelopScene::readSource(const Poco::LocalDateTime& now) {
	_log.information(Poco::format("check source[%s]: %s", Poco::DateTimeFormatter::format(now, Poco::DateTimeFormat::SORTABLE_FORMAT), _sourceURL));
	map<string, vector<string>> sources;
	Poco::XML::DOMParser parser;
	try {
		Poco::XML::Document* doc = parser.parse(_sourceURL);
		if (doc) {
			Poco::XML::Element* root = doc->documentElement();
			if (root->nodeName() == "rdf:RDF") {
				// rss1.0
				Poco::XML::NodeList* list = root->getElementsByTagName("channel");
				if (list) {
					Poco::XML::Element* e = (Poco::XML::Element*)list->item(0);
					Poco::XML::Element* title = e->getChildElement("title");
					_title = title->innerText();
					Poco::XML::Element* date = e->getChildElement("dc:date");
					if (date) {
						int tzd = 0;
						_date = Poco::DateTimeParser::parse(Poco::DateTimeFormat::ISO8601_FORMAT, date->innerText(), tzd);
					}
					list->release();
					list = NULL;
				}

				list = root->getElementsByTagName("item");
				if (list) {
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
				}

				int count = 0;
				for (map<string, vector<string>>::iterator m = sources.begin(); m != sources.end(); ++m) {
					//_log.information(Poco::format("category: %s",m->first));
					//for (vector<string>::iterator it = m->second.begin(); it != m->second.end(); ++it) {
					//	_log.information(Poco::format("text[%s]",*it));
					//}
					count += m->second.size();
				}
				_log.information(Poco::format("remote reading %ucategories, %dtexts", sources.size(), count));

			} else if (root->nodeName() == "rss") {
				// rss2.0 or atom
				Poco::XML::NodeList* channels = root->getElementsByTagName("channel");
				if (channels->length() > 0) {
					Poco::XML::Element* channel = (Poco::XML::Element*)channels->item(0);
					Poco::XML::Element* title = channel->getChildElement("title");
					_title = title->innerText();
					Poco::XML::Element* date = channel->getChildElement("pubDate");
					if (date) {
						int tzd = 0;
						_date = Poco::DateTimeParser::parse(Poco::DateTimeFormat::HTTP_FORMAT, date->innerText(), tzd);
					} else {
						date = channel->getChildElement("lastBuildDate");
						if (date) {
							int tzd = 0;
							_date = Poco::DateTimeParser::parse(Poco::DateTimeFormat::HTTP_FORMAT, date->innerText(), tzd);
						}
					}

					Poco::XML::NodeList* items = channel->getElementsByTagName("item");
					if (items->length() > 0) {
						for (int i = 0; i < items->length(); i++) {
							Poco::XML::Element* e = (Poco::XML::Element*)items->item(i);
							Poco::XML::Element* date = e->getChildElement("pubDate");
							int tzd = 0;
							Poco::DateTime dt;
							if (date) {
								dt = Poco::DateTimeParser::parse(Poco::DateTimeFormat::HTTP_FORMAT, date->innerText(), tzd);
								if (now.day() != dt.day()) continue;
								int d = (now.timestamp() - dt.timestamp()) / (60 * Poco::Timestamp::resolution());
								if (d > _validMinutes) continue;
								//_log.information(Poco::format("date: %s %d", date->innerText(), d));
							}
							Poco::XML::Element* title = e->getChildElement("title");
							if (title) {
								const string& key = "item";
								map<string, vector<string>>::iterator m = sources.find(key);
								if (m == sources.end()) {
									vector<string> vec;
									sources[key] = vec;
								}
								sources[key].push_back(title->innerText());
							}
						}
						items->release();
					}
					channels->release();
				}

				int count = 0;
				for (map<string, vector<string>>::iterator m = sources.begin(); m != sources.end(); ++m) {
					count += m->second.size();
				}
				_log.information(Poco::format("remote reading %ucategories, %dtexts", sources.size(), count));

			} else if (root->nodeName().find("-news") != string::npos) {
				// original format
				Poco::XML::NodeList* list = root->getElementsByTagName("item");
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
					}
				}
				list->release();

				int count = 0;
				for (map<string, vector<string>>::iterator m = sources.begin(); m != sources.end(); ++m) {
					count += m->second.size();
				}
				_log.information(Poco::format("remote reading %ucategories, %dtexts", sources.size(), count));
			}
			doc->release();
		} else {
			_log.warning("failed not parse remote XML");
		}
	} catch (Poco::IOException& ex) {
		_log.warning(Poco::format("failed not read XML: %s", ex.displayText()));
	} catch (Poco::PathSyntaxException& ex) {
		_log.warning(Poco::format("failed not read XML: %s", ex.displayText()));
	//} catch (...) {
	//	_log.warning(Poco::format("failed exception in readMXL(): %s", _remoteURL));
	}
	return sources;
}

void TelopScene::process() {
	switch (_keycode) {
	case 'T':
		_visible = !_visible;
		break;
	}

	if (!_visible) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		for (vector<Telop>::iterator it = _telops.begin(); it != _telops.end(); ) {
			Telop& t = *it;
			_deletes.push(t);
			it = _telops.erase(it);
		}
	} else if (_telops.empty()) {
		_creating = _visible;
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
	LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
	RECT rect;
	device->GetScissorRect(&rect);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
	RECT clip;
	device->SetScissorRect(&config().stageRect);
	DWORD col = 0xffffffff;
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		for (vector<Telop>::iterator it = _telops.begin(); it != _telops.end(); ++it) {
			Telop& t = *it;
			_renderer.drawTexture(t.x, t.y, t.texture, 0, col, col, col, col);
		}
	}
	device->SetScissorRect(&rect);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
}

void TelopScene::draw2() {
}
