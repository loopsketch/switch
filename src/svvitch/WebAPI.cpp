#include "WebAPI.h"

#include <Poco/DateTime.h>
#include <Poco/Timezone.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/NumberFormatter.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/XML/XMLWriter.h>
#include <Poco/format.h>
#include <Poco/RegularExpression.h>
#include <Poco/Net/HTMLForm.h>
#include <Poco/URI.h>
#include <Poco/FileStream.h>
#include <Poco/Path.h>
#include <Poco/StreamCopier.h>

#include "MainScene.h"
#include "Utils.h"

using Poco::RegularExpression;
using Poco::URI;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Text;
using Poco::XML::AutoPtr;
using Poco::XML::DOMWriter;
using Poco::XML::XMLWriter;


SwitchRequestHandlerFactory::SwitchRequestHandlerFactory(Renderer& renderer): _log(Poco::Logger::get("")), _renderer(renderer) {
	_log.information("create SwitchRequestHandlerFactory");
}

SwitchRequestHandlerFactory::~SwitchRequestHandlerFactory() {
	_log.information("release SwitchRequestHandlerFactory");
}

HTTPRequestHandler* SwitchRequestHandlerFactory::createRequestHandler(const HTTPServerRequest& request) {
	return new SwitchRequestHandler(_renderer);
}


SwitchPartHandler::SwitchPartHandler(): _log(Poco::Logger::get("")) {
	_log.debug("create SwitchPartHandler");
}

SwitchPartHandler::~SwitchPartHandler() {
	_log.debug("release SwitchPartHandler");
}

void SwitchPartHandler::handlePart(const MessageHeader& header, std::istream& is) {
	string type = header.get("Content-Type", "(unspecified)");
	if (header.has("Content-Disposition")) {
		string contentDisposition = header["Content-Disposition"]; // UTF-8ベースのページであればUTF-8になるようです
		string disp;
		Poco::Net::NameValueCollection params;
		MessageHeader::splitParameters(contentDisposition, disp, params);
		string name = params.get("name", "unnamed"); // formプロパティ名
		string fileName = params.get(name, "unnamed");
		_log.information(Poco::format("contentDisposition[%s] name[%s]", contentDisposition, name));
		Poco::RegularExpression re(".*filename=\"((.+\\\\)*(.+))\".*");
		if (re.match(contentDisposition)) {
			// IEのフルパス対策
			fileName = contentDisposition;
			re.subst(fileName, "$3");
			_log.information("fileName: " + fileName);
		}

		File uploadDir("uploads");
		if (!uploadDir.exists()) uploadDir.createDirectories();
		File f(uploadDir.path() + "/" + fileName + ".part");
		if (f.exists()) f.remove();
		try {
			Poco::FileOutputStream os(f.path());
			int size = Poco::StreamCopier::copyStream(is, os, 512 * 1024);
			os.close();
			File rename(uploadDir.path() + "/" + fileName);
			if (rename.exists()) rename.remove();
			f.renameTo(rename.path());
			_log.information(Poco::format("file %s %s %d", fileName, type, size));
		} catch (Poco::PathSyntaxException& ex) {
			_log.warning(Poco::format("failed download path[%s] %s", fileName, ex.displayText()));
		} catch (Poco::FileException& ex) {
			_log.warning(Poco::format("failed download file[%s] %s", fileName, ex.displayText()));
		}
	}
}


SwitchRequestHandler::SwitchRequestHandler(Renderer& renderer):
	_log(Poco::Logger::get("")), _renderer(renderer), _request(NULL), _response(NULL), _form(NULL) {
	_log.debug("create SwitchRequestHandler");
}

SwitchRequestHandler::~SwitchRequestHandler() {
	SAFE_DELETE(_form);
	_log.debug("release SwitchRequestHandler");
}

HTMLForm& SwitchRequestHandler::form() {
	if (!_form) {
		SwitchPartHandler partHandler;
		_form = new HTMLForm(request(), request().stream(), partHandler);
	}
	return *_form;
}

void SwitchRequestHandler::handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
	_request  = &request;
	_response = &response;

	_log.debug(Poco::format("contenttype %s", request.getContentType()));
	_log.debug(Poco::format("encoding %s", request.getTransferEncoding()));
	doRequest();
}

void SwitchRequestHandler::doRequest() {
	//_log.information(Poco::format("request from %s", request().clientAddress().toString()));
	string encoded;
	URI::encode(request().getURI(), "/", encoded);
	URI uri(encoded);
	//_log.information(Poco::format("webAPI access uri [%s]", uri.getPath()));
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
		} else if (urls[1] == "upload") {
			upload();
		} else if (urls[1] == "download") {
			download();
		} else if (urls[1] == "copy") {
			copy();
		} else if (urls[1] == "version") {
			version();
		}

	} else {
		_log.information(Poco::format("webAPI access uri [%s]", uri.getPath()));
		Path src("docs", uri.getPath().substr(1));
		File f(src);
		if (f.exists()) {
			if (f.isDirectory()) src = src.makeDirectory().append("index.html");
			sendFile(src);
		}
	}

	if (!response().sent()) {
		string s = Poco::format("<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL %s was not found on this server.</p><hr><address>switch %s</address></body></html>", request().getURI(), svvitch::version());
		sendResponse(HTTPResponse::HTTP_NOT_FOUND, s);
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
		MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer.getScene("main"));
		if (scene) {
			map<string, string> params;
			params["update"] = scene->updateWorkspace()?"true":"false";
			sendJSONP(form().get("callback", ""), params);
		} else {
			sendResponse(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "scene not found");
		}
	} catch (...) {
	}
}

void SwitchRequestHandler::set(const string& name) {
	try {
		string message;
		MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer.getScene("main"));
		if (scene) {
			if (name == "playlist") {
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
			} else if (name == "text") {
				string playlistID = form().get("pl", "");
				string text = form().get("t", "");
				map<string, string> params;
				params["text"] = scene->setPlaylistText(playlistID, text)?"true":"false";
				sendJSONP(form().get("callback", ""), params);
				return;

			} else if (name == "brightness") {
				int i = 0;
				Poco::NumberParser::tryParse(form().get("v"), i);
				scene->setBrightness(i);
				map<string, string> params;
				params["brightness"] = Poco::format("%d", i);
				sendJSONP(form().get("callback", ""), params);
				return;

			} else if (name == "action") {
				string action = form().get("v");
				scene->setAction(action);
				map<string, string> params;
				params["action"] = action;
				sendJSONP(form().get("callback", ""), params);
				return;

			} else if (name == "transition") {
				string transition = form().get("v");
				scene->setTransition(transition);
				map<string, string> params;
				params["transition"] = transition;
				sendJSONP(form().get("callback", ""), params);
				return;

			} else if (name == "status") {
				string name = form().get("n");
				string value = form().get("v");
				bool result = false;
				if (!name.empty()) {
					if (value.empty()) {
						scene->removeStatus(name);
					} else {
						scene->setStatus(name, value);
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
	}
}

void SwitchRequestHandler::get(const string& name) {
	try {
		string message;
		MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer.getScene("main"));
		if (scene) {
			if (name == "workspace") {
				response().sendFile("workspace.xml", "text/xml");
				return;

			} else if (name == "snapshot") {
				LPDIRECT3DTEXTURE9 capture = _renderer.getCaptureTexture();
				if (capture) {
					// capture-lock
					LPD3DXBUFFER buf = NULL;
					if SUCCEEDED(D3DXSaveTextureToFileInMemory(&buf, D3DXIFF_PNG, capture, NULL)) {
						//response().sendFile("snapshot.png", "image/png");
						try {
							response().setContentType("image/png");
							response().sendBuffer(buf->GetBufferPointer(), buf->GetBufferSize());
						} catch (...) {
						}
						SAFE_RELEASE(buf);
					} else {
						_log.warning("failed snapshot");
					}
				}
				return;

			} else if (name == "text") {
				string playlistID = form().get("pl", "");
				string text = scene->getPlaylistText(playlistID);
				Poco::RegularExpression re("\\r|\\n");
				re.subst(text, "\\n", Poco::RegularExpression::RE_GLOBAL);
				map<string, string> params;
				params["text"] = "\"" + text + "\"";
				sendJSONP(form().get("callback", ""), params);
				return;

			} else if (name == "status") {
				map<string, string> status = scene->getStatus();
				map<string, string> params;
				for (map<string, string>::const_iterator it = status.begin(); it != status.end(); it++) {
					params[it->first] = Poco::format("\"%s\"", it->second);
				}
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
	}
}

void SwitchRequestHandler::files() {
	string path = form().get("path", "");
	//_log.information(Poco::format("files: %s", path));
	Path dir = config().dataRoot;
	try {
		if (!path.empty()) dir = dir.append(path);
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
					files.push_back(Poco::format("\"%s/\"", subName));
				} else {
					files.push_back(Poco::format("\"%s\"", subName));
				}
			}
		}
		return svvitch::formatJSONArray(files);
	}
	map<string, string> params;
	params["name"] = "\"" + name + "\"";
	File f(path);
	Poco::DateTime modified(f.getLastModified());
	modified.makeLocal(Poco::Timezone::tzd());
	params["modified"] = "\"" + Poco::DateTimeFormatter::format(modified, Poco::DateTimeFormat::SORTABLE_FORMAT) + "\"";
	params["size"] = Poco::NumberFormatter::format(static_cast<int>(f.getSize()));
	//params["md5"] = "\"" + svvitch::md5(path) + "\"";
	return svvitch::formatJSON(params);
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
	if (!path.empty()) {
		form(); // フォームをパースしuploadsフォルダにアップロードファイルを取り込む
		try {
			if (path.at(0) == '/' || path.at(0) == '\\') path = path.substr(1);
			Path dst(config().dataRoot, Path(path).toString());
			File parent(dst.makeDirectory());
			if (!parent.exists()) parent.createDirectories();
			File f(dst);
			_log.information(Poco::format("upload: %s", f.path()));
			vector<File> list;
			File("uploads").list(list);
			boolean result = true;
			for (vector<File>::iterator it = list.begin(); it != list.end(); it++) {
				File& src = *it;
				try {
					if (f.exists()) {
						f.remove();
						_log.information(Poco::format("deleted already file: %s", f.path()));
					}
					src.renameTo(f.path());
					_log.information(Poco::format("rename: %s -> %s", src.path(), f.path()));
				} catch (Poco::FileException& ex) {
					_log.warning(Poco::format("failed upload file[%s]: %s", src.path(), ex.displayText()));
					MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer.getScene("main"));
					if (scene) {
						File tempFile(f.path() + ".part");
						if (tempFile.exists()) {
							scene->removeDelayedUpdateFile(tempFile);
							tempFile.remove();
						}
						try {
							src.renameTo(tempFile.path());
							scene->addDelayedUpdateFile(src);
						} catch (Poco::FileException& ex1) {
							result = false;
						}
					}
				}
			}
			map<string, string> params;
			params["upload"] = result?"true":"false";
			sendJSONP(form().get("callback", ""), params);
		} catch (Poco::PathSyntaxException& ex) {
			_log.warning(ex.displayText());
			sendResponse(HTTPResponse::HTTP_NOT_FOUND, ex.displayText());
		}
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

/** クライアントにファイルを送信します */
bool SwitchRequestHandler::sendFile(Path& path) {
	try {
		Poco::FileInputStream is(path.toString());
		if (is.good()) {
			string ext = path.getExtension();
			File f(path);
			File::FileSize length = f.getSize();
			response().setContentLength(static_cast<int>(length));
			if (ext == "png") {
				response().setContentType("image/png");
			} else if (ext == "jpg" || ext == "jpeg") {
				response().setContentType("image/jpeg");
			} else if (ext == "bmp") {
				response().setContentType("image/bmp");
			} else if (ext == "gif") {
				response().setContentType("image/gif");
			} else if (ext == "mpg" || ext == "mpeg") {
				response().setContentType("video/mpeg");
			} else if (ext == "mp4" || ext == "f4v" || ext == "264") {
				response().setContentType("video/mp4");
			} else if (ext == "wmv") {
				response().setContentType("video/x-ms-wmv"); 
			} else if (ext == "mov") {
				response().setContentType("video/quicktime");
			} else if (ext == "flv") {
				response().setContentType("video/x-flv");
			} else if (ext == "swf") {
				response().setContentType("application/x-shockwave-flash");
			} else if (ext == "pdf") {
				response().setContentType("application/pdf");
			} else if (ext == "txt") {
				response().setContentType("text/plain");
			} else if (ext == "htm" || ext == "html") {
				response().setContentType("text/html");
			} else if (ext == "xml") {
				response().setContentType("text/xml");
			} else {
				response().setContentType("application/octet-stream");
			}
			response().setChunkedTransferEncoding(false);
			Poco::StreamCopier::copyStream(is, response().send(), 512 * 1024);
			is.close();
			return true;
		} else {
			throw Poco::OpenFileException(path.toString());
		}
	} catch (Poco::FileException& ex) {
		_log.warning(ex.displayText());
		//sendResponse(HTTPResponse::HTTP_NOT_FOUND, ex.displayText());
	}
	return false;
}

void SwitchRequestHandler::sendJSONP(const string& functionName, const map<string, string>& json) {
	response().setChunkedTransferEncoding(true);
	response().setContentType("text/javascript; charset=UTF-8");
	response().add("CacheControl", "no-cache");
	response().add("Expires", "-1");
	if (functionName.empty()) {
		// コールバック関数名が無い場合はJSONとして送信
		response().send() << svvitch::formatJSON(json);
	} else {
		response().send() << Poco::format("%s(%s);", functionName, svvitch::formatJSON(json));
	}
}

void SwitchRequestHandler::writeResult(const int code, const string& description) {
	AutoPtr<Document> doc = new Document();
	AutoPtr<Element> remote = doc->createElement("remote");
	doc->appendChild(remote);
	AutoPtr<Element> result = doc->createElement("result");
	result->setAttribute("code", Poco::format("%d", code));
	AutoPtr<Text> resultText = doc->createTextNode(description);
	result->appendChild(resultText);
	remote->appendChild(result);
	DOMWriter writer;
	writer.setNewLine("\r\n");
	writer.setOptions(XMLWriter::WRITE_XML_DECLARATION | XMLWriter::PRETTY_PRINT);

	response().setChunkedTransferEncoding(true);
	response().setContentType("text/xml; charset=UTF-8");
	writer.writeNode(response().send(), doc);
}

void SwitchRequestHandler::sendResponse(HTTPResponse::HTTPStatus status, const string& message) {
	response().setStatusAndReason(status, message);

	string statusCode(Poco::NumberFormatter::format(static_cast<int>(response().getStatus())));
	response().setChunkedTransferEncoding(true);
	if (message.find("<html>") == string::npos) {
		response().setContentType("text/plain");
		response().send() << Poco::format("%s - %s", statusCode, message);
	} else {
		response().setContentType("text/html");
		response().send() << message;
	}
}
