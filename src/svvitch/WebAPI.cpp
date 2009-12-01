#include "WebAPI.h"

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


SwitchRequestHandler::SwitchRequestHandler(RendererPtr renderer): _log(Poco::Logger::get("")), _renderer(renderer) {
	_log.information("create SwitchRequestHandler");
}

SwitchRequestHandler::~SwitchRequestHandler() {
	_log.information("release SwitchRequestHandler");
}

void SwitchRequestHandler::run() {
	_log.information("request from " + request().clientAddress().toString());

	response().setChunkedTransferEncoding(true);
	response().setContentType("text/xml; charset=UTF-8");
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
			svvitch();
		} else {
			sendErrorResponse(HTTPResponse::HTTP_NOT_FOUND, Poco::format("not found command: %s", urls[0]));
		}
	} else {
		sendErrorResponse(HTTPResponse::HTTP_NOT_FOUND, request().getURI());
	}
}

void SwitchRequestHandler::set() {
	MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer->getScene("main"));
	if (scene) {
		string playlistID = form().get("pl");
		int playlistIndex = 0;
		if (form().has("i")) Poco::NumberParser::tryParse(form().get("i"), playlistIndex);
		if (!playlistID.empty()) {
			_log.information(Poco::format("playlist: %s", playlistID));
			bool result = scene->prepare(playlistID, playlistIndex);
			if (result) {
				writeResult(200, Poco::format("%s", playlistID));
			} else {
				writeResult(500, Poco::format("failed prepared %s", playlistID));
			}
		} else {
			writeResult(500, "empty playlist ID");
		}
	} else {
		writeResult(500, "scene not found");
	}
}

void SwitchRequestHandler::get() {
	writeResult(500, "not implemnted");
}

void SwitchRequestHandler::svvitch() {
	MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer->getScene("main"));
	if (scene) {
		scene->switchContent();
		writeResult(200, "switch content");
	} else {
		writeResult(500, "scene not found");
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
	writer.writeNode(response().send(), doc);
}

