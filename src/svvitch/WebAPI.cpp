#include "WebAPI.h"

#include <Poco/NumberFormatter.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/XML/XMLWriter.h>
#include <Poco/File.h>
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


SwitchRequestHandlerFactory::SwitchRequestHandlerFactory(RendererPtr renderer): _log(Poco::Logger::get("")), _renderer(renderer) {
}

SwitchRequestHandlerFactory::~SwitchRequestHandlerFactory() {
	_log.information("release SwitchRequestHandlerFactory");
}

HTTPRequestHandler* SwitchRequestHandlerFactory::createRequestHandler(const HTTPServerRequest& request) {
	return new SwitchRequestHandler(_renderer);
}


SwitchPartHandler::SwitchPartHandler(): _log(Poco::Logger::get("")) {
	_log.information("create SwitchPartHandler");
}

SwitchPartHandler::~SwitchPartHandler() {
	_log.information("release SwitchPartHandler");
}

void SwitchPartHandler::handlePart(const MessageHeader& header, std::istream& is) {
	string type = header.get("Content-Type", "(unspecified)");
	if (header.has("Content-Disposition")) {
		string contentDisposition = header["Content-Disposition"]; // UTF-8ベースのページであればUTF-8になるようです
		string disp;
		Poco::Net::NameValueCollection params;
		MessageHeader::splitParameters(contentDisposition, disp, params);
		string name = params.get("name", "(unnamed)"); // formプロパティ名
		string fileName = params.get("filename", "unnamed");
		Poco::RegularExpression re(".*filename=\"((.+\\\\)*(.+))\".*");
		if (re.match(contentDisposition)) {
			// IEのフルパス対策
			fileName = contentDisposition;
			re.subst(fileName, "$3");
			_log.information("fileName: " + fileName);
		}

		Poco::File rootDir("uploads");
		if (!rootDir.exists()) rootDir.createDirectories();
		Poco::File f(rootDir.path() + "/" + fileName + ".part");
		try {
			Poco::FileOutputStream os(f.path());
			int size = Poco::StreamCopier::copyStream(is, os, 512 * 1024);
			os.close();
			Poco::File rename(rootDir.path() + "/" + fileName);
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


SwitchRequestHandler::SwitchRequestHandler(RendererPtr renderer):
	_log(Poco::Logger::get("")), _renderer(renderer), _request(NULL), _response(NULL), _form(NULL) {
	_log.information("create SwitchRequestHandler");
}

SwitchRequestHandler::~SwitchRequestHandler() {
	SAFE_DELETE(_form);
	_log.information("release SwitchRequestHandler");
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

	_log.information(Poco::format("contenttype %s", request.getContentType()));
	_log.information(Poco::format("encoding %s", request.getTransferEncoding()));
	doRequest();
}

void SwitchRequestHandler::doRequest() {
	_log.information(Poco::format("request from %s", request().clientAddress().toString()));
	URI uri(request().getURI());
	vector<string> urls;
	svvitch::split(uri.getPath().substr(1), '/', urls);
	int count = urls.size();
	if (!urls.empty()) {
		if        (urls[0] == "set") {
			if (urls.size() > 1) set(urls[1]);
		} else if (urls[0] == "get") {
			if (urls.size() > 1) get(urls[1]);
		} else if (urls[0] == "switch") {
			switchContent();
		}
	}

	if (!response().sent()) {
		sendResponse(HTTPResponse::HTTP_NOT_FOUND, Poco::format("not found: %s", request().getURI()));
	}
}

void SwitchRequestHandler::set(const string& name) {
	string message;
	MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer->getScene("main"));
	if (scene) {
		if (name == "playlist") {
			string playlistID = form().get("pl", "");
			int playlistIndex = 0;
			if (form().has("i")) Poco::NumberParser::tryParse(form().get("i"), playlistIndex);
			_log.information(Poco::format("set playlist: [%s]-%d", playlistID, playlistIndex));
			_log.information(Poco::format("playlist: %s", playlistID));
			bool result = scene->prepare(playlistID, playlistIndex);
			if (result) {
				map<string, string> params;
				params["playlist"] = Poco::format("\"%s\"", playlistID);
				params["index"] = Poco::format("%d", playlistIndex);
				sendJSONP(form().get("callback", ""), params);
				return;
			} else {
				message = Poco::format("failed prepared [%s]", playlistID);
			}
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
	MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer->getScene("main"));
	if (scene) {
		Workspace& workspace = scene->getWorkspace();
		if (name == "playlist") {
			string playlistID = form().get("pl", "");
			vector<string> playlists;
			for (int i = 0; i < workspace.getPlaylistCount();i ++) {
				PlayListPtr playlist = workspace.getPlaylist(i);
				if (!playlistID.empty() && playlistID != playlist->id()) continue;
				map<string, string> params;
				params["id"] = Poco::format("\"%s\"", playlist->id());
				vector<string> ids;
				const vector<PlayListItemPtr> items = playlist->items();
				for (vector<PlayListItemPtr>::const_iterator it = items.begin(); it != items.end(); it++) {
					ids.push_back(Poco::format("\"%s\"", (*it)->media()->id()));
				}
				string itemJSON;
				svvitch::formatJSONArray(ids, itemJSON);
				params["items"] = itemJSON;
				string playlistJSON;
				svvitch::formatJSON(params, playlistJSON);
				playlists.push_back(playlistJSON);
			}
			map<string, string> result;
			string playlistsJSON;
			svvitch::formatJSONArray(playlists, playlistsJSON);
			result["playlists"] = playlistsJSON;
			sendJSONP(form().get("callback", ""), result);
			return;
		} else {
			message = "property not found";
		}
	} else {
		message = "scene not found";
	}
	sendResponse(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "scene not found");
}

void SwitchRequestHandler::switchContent() {
	MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer->getScene("main"));
	if (scene) {
		scene->switchContent();
		map<string, string> params;
		params["switched"] = "true";
		sendJSONP(form().get("callback", ""), params);
	} else {
		sendResponse(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "scene not found");
	}
}

void SwitchRequestHandler::sendJSONP(const string& functionName, const map<string, string>& json) {
	string params;
	svvitch::formatJSON(json, params);
	response().send() << Poco::format("%s(%s);", functionName, params);
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

//	response().setStatus(HTTPResponse::HTTP_OK);
//	response().setChunkedTransferEncoding(true);
//	response().setContentType("text/xml");
	writer.writeNode(response().send(), doc);
}

void SwitchRequestHandler::sendResponse(HTTPResponse::HTTPStatus status, const string& message) {
	response().setStatusAndReason(status, message);

	string statusCode(Poco::NumberFormatter::format(static_cast<int>(response().getStatus())));
//	response().setChunkedTransferEncoding(true);
//	response().setContentType("text/plain");
	response().send() << Poco::format("%s - %s", statusCode, message);
}
