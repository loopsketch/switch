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
	_log.debug(Poco::format("request from %s", request().clientAddress().toString()));
	URI uri(request().getURI());
	vector<string> urls;
	svvitch::split(uri.getPath().substr(1), '/', urls, 2);
//	for (vector<string>::iterator it = urls.begin(); it != urls.end(); it++) {
//		_log.information(Poco::format("url %s", string(*it)));
//	}
	int count = urls.size();
	if (!urls.empty()) {
		if        (urls[0] == "switch") {
			switchContent();
		} else if (urls[0] == "update") {
			updateWorkspace();
		} else if (urls[0] == "set") {
			if (urls.size() == 2) set(urls[1]);
		} else if (urls[0] == "get") {
			if (urls.size() == 2) get(urls[1]);
		} else if (urls[0] == "files") {
			files();
		} else if (urls[0] == "upload") {
			upload();
		} else if (urls[0] == "download") {
			download();
		}
	}

	if (!response().sent()) {
		sendResponse(HTTPResponse::HTTP_NOT_FOUND, Poco::format("not found: %s", request().getURI()));
	}
}

void SwitchRequestHandler::switchContent() {
	MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer.getScene("main"));
	if (scene) {
		map<string, string> params;
		params["switched"] = scene->switchContent()?"true":"false";
		sendJSONP(form().get("callback", ""), params);
	} else {
		sendResponse(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "scene not found");
	}
}

void SwitchRequestHandler::updateWorkspace() {
	MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer.getScene("main"));
	if (scene) {
		map<string, string> params;
		params["update"] = scene->updateWorkspace()?"true":"false";
		sendJSONP(form().get("callback", ""), params);
	} else {
		sendResponse(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "scene not found");
	}
}

void SwitchRequestHandler::set(const string& name) {
	string message;
	MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer.getScene("main"));
	if (scene) {
		if (name == "playlist") {
			string playlistID = form().get("pl", "");
			int playlistIndex = 0;
			if (form().has("i")) Poco::NumberParser::tryParse(form().get("i"), playlistIndex);
			_log.information(Poco::format("set playlist: [%s]-%d", playlistID, playlistIndex));
			_log.information(Poco::format("playlist: %s", playlistID));
			bool result = scene->stackPrepare(playlistID, playlistIndex);
			map<string, string> params;
			if (result) {
				Workspace& workspace = scene->getWorkspace();
				PlayListPtr playlist = workspace.getPlaylist(playlistID);
				if (playlist) params["name"] = Poco::format("\"%s\"", playlist->name());
				params["playlist"] = Poco::format("\"%s\"", playlistID);
				params["index"] = Poco::format("%d", playlistIndex);
			}
			sendJSONP(form().get("callback", ""), params);
			return;
		} else if (name == "text") {
			string playlistID = form().get("pl", "");
			Workspace& workspace = scene->getWorkspace();
			PlayListPtr playlist = workspace.getPlaylist(playlistID);
			playlist->text(form().get("t", ""));
			map<string, string> params;
			params["text"] = Poco::format("\"%s\"", playlist->text());
			sendJSONP(form().get("callback", ""), params);
			return;
		} else {
			message = "property not found";
		}
	} else {
		message = "scene not found";
	}
	sendResponse(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, message);
}

void SwitchRequestHandler::get(const string& name) {
	string message;
	MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer.getScene("main"));
	if (scene) {
		Workspace& workspace = scene->getWorkspace();
		if (name == "workspace") {
			response().sendFile("workspace.xml", "text/xml");
			return;

		} else if (name == "snapshot") {
			LPDIRECT3DTEXTURE9 capture = _renderer.getCaptureTexture();
			if (capture) {
				// capture-lock
				if SUCCEEDED(D3DXSaveTextureToFile(L"snapshot.png", D3DXIFF_PNG, capture, NULL)) {
					response().sendFile("snapshot.png", "image/png");
				} else {
					_log.warning("failed snapshot");
				}
			}
			return;

		} else if (name == "playlist") {
			string playlistID = form().get("pl", "");
			vector<string> playlists;
			for (int i = 0; i < workspace.getPlaylistCount();i ++) {
				PlayListPtr playlist = workspace.getPlaylist(i);
				if (!playlistID.empty() && playlistID != playlist->id()) continue;
				map<string, string> params;
				params["id"] = Poco::format("\"%s\"", playlist->id());
				params["name"] = Poco::format("\"%s\"", playlist->name());
				vector<string> ids;
				const vector<PlayListItemPtr> items = playlist->items();
				for (vector<PlayListItemPtr>::const_iterator it = items.begin(); it != items.end(); it++) {
					ids.push_back(Poco::format("\"%s\"", (*it)->media()->id()));
				}
				params["items"] = svvitch::formatJSONArray(ids);
				playlists.push_back(svvitch::formatJSON(params));
			}
			map<string, string> result;
			result["playlists"] = svvitch::formatJSONArray(playlists);
			sendJSONP(form().get("callback", ""), result);
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
	sendResponse(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "scene not found");
}

void SwitchRequestHandler::files() {
	string path = form().get("path", "");
	Path dir = config().dataRoot;
	try {
		if (!path.empty()) dir = dir.append(path);
		_log.information(Poco::format("files: %s", dir.toString()));
		map<string, string> result;
		result["files"] = fileToJSON(dir);
		sendJSONP(form().get("callback", ""), result);
	} catch (Poco::FileException ex) {
		_log.warning(ex.displayText());
		sendResponse(HTTPResponse::HTTP_NOT_FOUND, ex.displayText());
	} catch (Poco::PathSyntaxException ex) {
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
			string subName = Path((*it).path()).getFileName();
			if (subName.length() > 1 && subName.at(0) != '.' && subName.at(0) != '$') {
				files.push_back(Poco::format("\"%s\"", subName));
			}
		}
		return svvitch::formatJSONArray(files);
	}
	map<string, string> params;
	params["name"] = Poco::format("\"%s\"", name);
	File f(path);
	Poco::DateTime modified(f.getLastModified());
	modified.makeLocal(Poco::Timezone::tzd());
	params["modified"] = "\"" + Poco::DateTimeFormatter::format(modified, Poco::DateTimeFormat::SORTABLE_FORMAT) + "\"";
	params["size"] = Poco::NumberFormatter::format(static_cast<int>(f.getSize()));
	params["md5"] = svvitch::md5(path);
	return svvitch::formatJSON(params);
}

void SwitchRequestHandler::download() {
	try {
		string path = form().get("path", "");
		if (path.at(0) == '/' || path.at(0) == '\\') path = path.substr(1);
		Path src(config().dataRoot, Path(path).toString());
		File f(src);
		_log.information(Poco::format("download: %s", f.path()));
		Poco::FileInputStream is(src.toString());
		if (is.good()) {
			string ext = src.getExtension();
			File::FileSize length = f.getSize();
			response().setContentLength(static_cast<int>(length));
			if (ext == "png") {
				response().setContentType("image/png");
			} else if (ext == "jpg" || ext == "jpeg") {
				response().setContentType("image/jpeg");
			} else if (ext == "bmp") {
				response().setContentType("image/bmp");
			} else if (ext == "mpg" || ext == "mpeg") {
				response().setContentType("video/mpeg");
			} else if (ext == "mp4" || ext == "f4v" || ext == "264") {
				response().setContentType("video/mp4");
			} else if (ext == "mov") {
				response().setContentType("video/quicktime");
			} else if (ext == "flv") {
				response().setContentType("video/x-flv");
			} else if (ext == "swf") {
				response().setContentType("application/x-shockwave-flash");
			} else if (ext == "txt") {
				response().setContentType("text/plain");
			} else if (ext == "xml") {
				response().setContentType("text/xml");
			} else {
				response().setContentType("application/octet-stream");
			}
			response().setChunkedTransferEncoding(false);
			Poco::StreamCopier::copyStream(is, response().send());
		} else {
			throw Poco::OpenFileException(src.toString());
		}
	} catch (Poco::FileException ex) {
		_log.warning(ex.displayText());
		sendResponse(HTTPResponse::HTTP_NOT_FOUND, ex.displayText());
	} catch (Poco::PathSyntaxException ex) {
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
			File parent(dst.parent());
			if (!parent.exists()) parent.createDirectories();
			File f(dst);
			_log.information(Poco::format("upload: %s", f.path()));
			vector<File> list;
			File("uploads").list(list);
			boolean result = true;
			for (vector<File>::iterator it = list.begin(); it != list.end(); it++) {
				try {
					File src = *it;
					if (f.exists()) {
						f.remove();
						_log.information(Poco::format("deleted already file: %s", f.path()));
					}
					src.renameTo(f.path());
					_log.information(Poco::format("rename: %s -> %s", src.path(), f.path()));
				} catch (Poco::FileException ex) {
					_log.warning(ex.displayText());
					result = false;
				}
			}
			map<string, string> params;
			params["upload"] = result?"true":"false";
			sendJSONP(form().get("callback", ""), params);
		} catch (Poco::PathSyntaxException ex) {
			_log.warning(ex.displayText());
			sendResponse(HTTPResponse::HTTP_NOT_FOUND, ex.displayText());
		}
	}
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
	response().setContentType("text/plain");
	response().send() << Poco::format("%s - %s", statusCode, message);
}
