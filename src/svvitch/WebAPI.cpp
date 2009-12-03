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
			set();
		} else if (urls[0] == "get") {
			get();
		} else if (urls[0] == "switch") {
			switchContent();
		}
	}

	if (!response().sent()) {
		sendResponse(HTTPResponse::HTTP_NOT_FOUND, Poco::format("not found: %s", request().getURI()));
	}
}

void SwitchRequestHandler::set() {
	MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer->getScene("main"));
	if (scene) {
		_log.information(Poco::format("name: [%s]", form().get("name", "none")));
		string playlistID = form().get("pl", "");
		int playlistIndex = 0;
		if (form().has("i")) Poco::NumberParser::tryParse(form().get("i"), playlistIndex);
		if (!playlistID.empty()) {
			_log.information(Poco::format("playlist: %s", playlistID));
			bool result = scene->prepare(playlistID, playlistIndex);
			if (result) {
				writeResult(200, Poco::format("%s", playlistID));
			} else {
				sendResponse(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, Poco::format("failed prepared %s", playlistID));
			}
		} else {
			sendResponse(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "empty playlist ID");
		}
	} else {
		sendResponse(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "scene not found");
	}
}

void SwitchRequestHandler::get() {
	sendResponse(HTTPResponse::HTTP_NOT_IMPLEMENTED, "not implemented");
}

void SwitchRequestHandler::switchContent() {
	response().setChunkedTransferEncoding(true);
	response().setContentType("text/xml; charset=UTF-8");
	MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer->getScene("main"));
	if (scene) {
		scene->switchContent();
		writeResult(200, "switch content");
	} else {
		sendResponse(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "scene not found");
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
	writer.setNewLine("\n");
	writer.setOptions(XMLWriter::PRETTY_PRINT);

	response().setChunkedTransferEncoding(true);
	response().setContentType("text/xml; charset=UTF-8");
	writer.writeNode(response().send(), doc);
}

void SwitchRequestHandler::sendResponse(HTTPResponse::HTTPStatus status, const string& message) {
	response().setStatusAndReason(status, message);

	string statusCode(Poco::NumberFormatter::format(static_cast<int>(response().getStatus())));
	response().send() << Poco::format("%s - %s", statusCode, message);
}
