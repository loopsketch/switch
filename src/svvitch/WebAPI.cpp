#include "WebAPI.h"

#include <Poco/DateTime.h>
#include <Poco/format.h>
#include <Poco/Timezone.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeParser.h>
#include <Poco/NumberFormatter.h>
#include <Poco/RegularExpression.h>
#include <Poco/URI.h>

#include "MainScene.h"
#include "CaptureScene.h"
#include "Utils.h"


SwitchRequestHandlerFactory::SwitchRequestHandlerFactory(Renderer& renderer):
	_log(Poco::Logger::get("")), _renderer(renderer)
{
	_log.information("create SwitchRequestHandlerFactory");
}

SwitchRequestHandlerFactory::~SwitchRequestHandlerFactory() {
	_log.information("release SwitchRequestHandlerFactory");
}

HTTPRequestHandler* SwitchRequestHandlerFactory::createRequestHandler(const HTTPServerRequest& request) {
	return new SwitchRequestHandler(_renderer);
}



SwitchRequestHandler::SwitchRequestHandler(Renderer& renderer): BaseRequestHandler(),
	_renderer(renderer)
{
	_log.debug("create SwitchRequestHandler");
}

SwitchRequestHandler::~SwitchRequestHandler() {
	_log.debug("release SwitchRequestHandler");
}


void SwitchRequestHandler::doRequest() {
	//_log.information(Poco::format("request from %s", request().clientAddress().toString()));
	string encoded;
	Poco::URI::encode(request().getURI(), "/", encoded);
	Poco::URI uri(encoded);
	_log.information(Poco::format("webAPI access uri [%s]", uri.getPath()));
	//vector<string> params;
	//svvitch::split(uri.getPath().substr(1), '?', params, 2);
	vector<string> urls;
	svvitch::split(uri.getPath().substr(1), '/', urls, 3);
	// for (vector<string>::iterator it = urls.begin(); it != urls.end(); it++) {
	//	_log.information(Poco::format("url %s", string(*it)));
	// }
	if (urls.size() > 1) {
		string displayID = urls[0];
		if        (urls[1] == "switch") {
			switchContent();
		} else if (urls[1] == "update") {
			updateWorkspace();
		} else if (urls[1] == "set") {
			if (urls.size() == 3) set(urls[2]);
		} else if (urls[1] == "get") {
			if (urls.size() == 3) get(urls[2]);
		} else if (urls[1] == "files") {
			files();
		} else if (urls[1] == "download") {
			download();
		} else if (urls[1] == "upload") {
			upload();
		} else if (urls[1] == "clear-stock") {
			clearStock();
		} else if (urls[1] == "copy") {
			copy();

		//} else if (urls[1] == "reboot") {
		//	svvitch::rebootWindows();
		//	sendResponse(HTTPResponse::HTTP_OK, "reboot now");

		} else if (urls[1] == "version") {
			version();
		}
	}

	if (!response().sent()) {
		Path src("docs", uri.getPath().substr(1));
		File f(src);
		if (f.isDirectory()) {
			src = src.makeDirectory().append("index.html");
			f = File(src);
		}
		if (f.exists()) {
			sendFile(src);
		} else {
			_log.warning(Poco::format("failed access uri [%s]", uri.getPath()));
			string s = Poco::format("<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL %s was not found on this server.</p><hr><address>switch %s</address></body></html>", request().getURI(), svvitch::version());
			sendResponse(HTTPResponse::HTTP_NOT_FOUND, s);
		}
	}
}

void SwitchRequestHandler::switchContent() {
	try {
		MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer.getScene("main"));
		if (scene) {
			map<string, string> params;
			params["switched"] = scene->switchContent()?"true":"false";
			sendJSONP(form().get("callback", ""), params);
		} else {
			sendResponse(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "scene not found");
		}
	} catch (...) {
	}
}

void SwitchRequestHandler::updateWorkspace() {

	try {
		MainScenePtr main = dynamic_cast<MainScenePtr>(_renderer.getScene("main"));
		if (main) {
			bool result = main->flushStock() && main->updateWorkspace();
			map<string, string> params;
			params["update"] = result?"true":"false";
			sendJSONP(form().get("callback", ""), params);
		} else {
			sendResponse(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "scene not found");
		}
	} catch (...) {
	}
}

void SwitchRequestHandler::set(const string& command) {
	try {
		string message;
		MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer.getScene("main"));
		if (scene) {
			if (command == "playlist") {
				string playlistID = form().get("pl", "");
				int playlistIndex = 0;
				if (form().has("i")) Poco::NumberParser::tryParse(form().get("i"), playlistIndex);
				_log.information(Poco::format("set playlist: [%s]-%d", playlistID, playlistIndex));
				_log.information(Poco::format("playlist: %s", playlistID));
				bool result = scene->stackPrepareContent(playlistID, playlistIndex);
				map<string, string> params;
				params["playlist"] = result?"true":"false";
				if (result) {
					//Workspace& workspace = scene->getWorkspace();
					//PlayListPtr playlist = workspace.getPlaylist(playlistID);
					//if (playlist) params["name"] = Poco::format("\"%s\"", playlist->name());
					//params["playlist"] = Poco::format("\"%s\"", playlistID);
					//params["index"] = Poco::format("%d", playlistIndex);
				}
				sendJSONP(form().get("callback", ""), params);
				return;
			} else if (command == "text") {
				string playlistID = form().get("pl", "");
				string text = form().get("t", "");
				map<string, string> params;
				params["text"] = scene->setPlaylistText(playlistID, text)?"true":"false";
				sendJSONP(form().get("callback", ""), params);
				return;

			} else if (command == "brightness") {
				int i = 0;
				Poco::NumberParser::tryParse(form().get("v"), i);
				scene->setBrightness(i);
				map<string, string> params;
				params["brightness"] = Poco::format("%d", i);
				sendJSONP(form().get("callback", ""), params);
				return;

			} else if (command == "pause") {
				string pause = form().get("v");
				scene->setPause(pause == "true");
				map<string, string> params;
				params["pause"] = pause;
				sendJSONP(form().get("callback", ""), params);
				return;

			} else if (command == "action") {
				string action = form().get("v");
				scene->setAction(action);
				map<string, string> params;
				params["action"] = action;
				sendJSONP(form().get("callback", ""), params);
				return;

			} else if (command == "transition") {
				string transition = form().get("v");
				scene->setTransition(transition);
				map<string, string> params;
				params["transition"] = transition;
				sendJSONP(form().get("callback", ""), params);
				return;

			} else if (command == "status") {
				bool result = false;
				string name = form().get("n");
				string value = form().get("v");
				ScenePtr targetScene = scene;
				string s = form().get("s", "");
				if (!s.empty()) {
					targetScene = _renderer.getScene(s);
				}
				if (!name.empty()) {
					if (value.empty()) {
						targetScene->removeStatus(name);
					} else {
						targetScene->setStatus(name, value);
					}
					result = true;
				}
				map<string, string> params;
				params["status"] = result?"true":"false";
				sendJSONP(form().get("callback", ""), params);
				return;

			} else {
				message = "property not found";
			}
		} else {
			message = "scene not found";
		}
		sendResponse(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, message);
	} catch (...) {
		sendResponse(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, Poco::format("failed set '%s'", command));
	}
}

void SwitchRequestHandler::get(const string& command) {
	try {
		string message;
		MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer.getScene("main"));
		if (scene) {
			if (command == "snapshot") {
				LPDIRECT3DTEXTURE9 texture = _renderer.getCaptureTexture();
				if (texture) {
					// capture-lock
					LPD3DXBUFFER buf = NULL;
					if SUCCEEDED(D3DXSaveTextureToFileInMemory(&buf, D3DXIFF_PNG, texture, NULL)) {
						try {
							response().setContentType("image/png");
							response().sendBuffer(buf->GetBufferPointer(), buf->GetBufferSize());
						} catch (Poco::Exception& ex) {
						}
						SAFE_RELEASE(buf);
					} else {
						_log.warning("failed snapshot image");
					}
				}
				return;

			} else if (command == "camera") {
				CaptureScenePtr capture = dynamic_cast<CaptureScenePtr>(_renderer.getScene("capture"));
				if (capture) {
					LPDIRECT3DTEXTURE9 texture = capture->getCameraImage();
					if (texture) {
						// capture-lock
						LPD3DXBUFFER buf = NULL;
						if SUCCEEDED(D3DXSaveTextureToFileInMemory(&buf, D3DXIFF_PNG, texture, NULL)) {
							try {
								response().setContentType("image/png");
								response().sendBuffer(buf->GetBufferPointer(), buf->GetBufferSize());
							} catch (Poco::Exception& ex) {
							}
							SAFE_RELEASE(buf);
						} else {
							_log.warning("failed snapshot image");
						}
					}
				}
				return;

			} else if (command == "fonts") {
				vector<string> fonts;
				scene->renderer().getPrivateFontFamilies(fonts);
				//vector<string> arrays;
				//for (vector<string>::iterator it = fonts.begin(); it != fonts.end(); it++) {
				//}

				map<string, string> params;
				params["fonts"] = svvitch::formatJSONArray(fonts);
				sendJSONP(form().get("callback", ""), params);
				return;

			} else if (command == "text") {
				string playlistID = form().get("pl", "");
				string text = scene->getPlaylistText(playlistID);
				Poco::RegularExpression re("\\r|\\n");
				re.subst(text, "\\n", Poco::RegularExpression::RE_GLOBAL);
				map<string, string> params;
				params["text"] = "\"" + text + "\"";
				sendJSONP(form().get("callback", ""), params);
				return;

			} else if (command == "display-status") {
				map<string, string> status = scene->getStatus();
				map<string, string>::const_iterator it = status.find("remote-copy");
				map<string, string> params;
				if (it != status.end()) {
					params[it->first] = Poco::format("\"%s\"", it->second);
				}
				sendJSONP(form().get("callback", ""), params);
				return;

			} else if (command == "status") {
				ScenePtr targetScene = dynamic_cast<MainScenePtr>(scene);
				string s = form().get("s", "");
				string name = form().get("n", "");
				if (!s.empty()) {
					targetScene = _renderer.getScene(s);
					if (!targetScene) {
						sendResponse(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "scene not found");
						return;
					}
				}
				map<string, string> status = targetScene->getStatus();
				if (!name.empty()) {
					map<string, string>::const_iterator it = status.find(name);
					string value;
					if (it != status.end()) value = it->second;
					sendResponse(HTTPResponse::HTTP_OK, value);
				} else {
					map<string, string> params;
					for (map<string, string>::const_iterator it = status.begin(); it != status.end(); it++) {
						params[it->first] = svvitch::formatJSON(it->second);
					}
					sendJSONP(form().get("callback", ""), params);
				}
				return;

			} else {
				message = "property not found";
			}
		} else {
			message = "scene not found";
		}
		sendResponse(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, message);
	} catch (...) {
		sendResponse(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, Poco::format("failed get '%s'", command));
	}
}

void SwitchRequestHandler::files() {
	string path = form().get("path", "");
	if (path.at(0) == '/' || path.at(0) == '\\') path = path.substr(1);
	//_log.information(Poco::format("files: %s", path));
	try {
		Path dir(config().dataRoot, Path(path).toString());
		//Path dir = config().dataRoot;
		//if (!path.empty()) dir = dir.append(path);
		map<string, string> result;
		result["count"] = Poco::format("%d", svvitch::fileCount(dir));
		result["path"] = "\"" + path + "\"";
		result["files"] = fileToJSON(dir);
		sendJSONP(form().get("callback", ""), result);
	} catch (Poco::FileException& ex) {
		_log.warning(ex.displayText());
		sendResponse(HTTPResponse::HTTP_NOT_FOUND, ex.displayText());
	} catch (Poco::PathSyntaxException& ex) {
		_log.warning(ex.displayText());
		sendResponse(HTTPResponse::HTTP_NOT_FOUND, ex.displayText());
	}
}

string SwitchRequestHandler::fileToJSON(const Path path) {
	try {
		string name = path.getFileName();
		if (name.length() > 1 && (name.at(0) == '.' || name.at(0) == '$')) return "";

		if (path.isDirectory()) {
			vector<string> files;
			vector<File> list;
			File(path).list(list);
			for (vector<File>::iterator it = list.begin(); it != list.end(); it++) {
	//			string json = fileToJSON(*it);
	//			files.push_back(json);
				File f = *it;
				string subName = Path(f.path()).getFileName();
				if (subName.length() > 1 && subName.at(0) != '.' && subName.at(0) != '$') {
					if (f.isDirectory()) {
						files.push_back(subName + "/");
					} else {
						files.push_back(subName);
					}
				}
			}
			return svvitch::formatJSONArray(files);
		}
		map<string, string> params;
		params["name"] = svvitch::formatJSON(name);
		File f(path);
		Poco::DateTime modified(f.getLastModified());
		modified.makeLocal(Poco::Timezone::tzd());
		params["modified"] = "\"" + Poco::DateTimeFormatter::format(modified, Poco::DateTimeFormat::SORTABLE_FORMAT) + "\"";
		params["size"] = Poco::format("%Lu", f.getSize());
		//if (f.getSize() < 10485760) {
		//	// 10MB以下ならmd5を計算
		//	params["md5"] = "\"" + svvitch::md5(path) + "\"";
		//}
		return svvitch::formatJSON(params);
	} catch (Poco::FileException& ex) {
		_log.warning(ex.displayText());
		sendResponse(HTTPResponse::HTTP_NOT_FOUND, ex.displayText());
	} catch (Poco::PathSyntaxException& ex) {
		_log.warning(ex.displayText());
		sendResponse(HTTPResponse::HTTP_NOT_FOUND, ex.displayText());
	}
}

void SwitchRequestHandler::download() {
	try {
		string path = form().get("path", "");
		if (path.at(0) == '/' || path.at(0) == '\\') path = path.substr(1);
		Path src(config().dataRoot, Path(path).toString());
		_log.information(Poco::format("download: %s", src.toString()));
		sendFile(src);
	} catch (Poco::PathSyntaxException& ex) {
		_log.warning(ex.displayText());
		sendResponse(HTTPResponse::HTTP_NOT_FOUND, ex.displayText());
	}
}

void SwitchRequestHandler::upload() {
	string path = form().get("path", "");
	string modified = form().get("modified", "");
	if (!path.empty()) {
		form(); // フォームをパースしuploadsフォルダにアップロードファイルを取り込む
		if (path.at(0) == '/' || path.at(0) == '\\') path = path.substr(1);
		boolean result = true;
		MainScenePtr main = dynamic_cast<MainScenePtr>(_renderer.getScene("main"));
		if (main) {
			vector<File> list;
			File("uploads").list(list);
			for (vector<File>::iterator it = list.begin(); it != list.end(); it++) {
				File src = *it;
				if (!modified.empty()) {
					int tz = 0;
					Poco::DateTime time;
					Poco::DateTimeParser::parse(Poco::DateTimeFormat::SORTABLE_FORMAT, modified, time, tz);
					src.setLastModified(time.timestamp());
				}
				result &= main->addStock(path, src);
			}
		}
		map<string, string> params;
		params["upload"] = result?"true":"false";
		sendJSONP(form().get("callback", ""), params);
	} else {
		sendResponse(HTTPResponse::HTTP_NOT_FOUND, "path not found");
	}
}

void SwitchRequestHandler::clearStock() {
	MainScenePtr main = dynamic_cast<MainScenePtr>(_renderer.getScene("main"));
	if (main) {
		main->clearStock();

		map<string, string> params;
		params["clear"] = "true";
		sendJSONP(form().get("callback", ""), params);
	}
}


void SwitchRequestHandler::copy() {
	string remote = form().get("remote", "");
	if (!remote.empty()) {
		MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer.getScene("main"));
		if (scene) {
			scene->activeCopyRemote(remote);

			map<string, string> params;
			params["copy"] = "true";
			sendJSONP(form().get("callback", ""), params);
		}
	}
}

void SwitchRequestHandler::version() {
	map<string, string> params;
	params["version"] = svvitch::version();
	sendJSONP(form().get("callback", ""), params);
}
