#include "WebAPI.h"

#include <Poco/format.h>
#include <Poco/RegularExpression.h>
#include <Poco/Net/HTMLForm.h>
#include <Poco/URI.h>

#include "MainScene.h"
#include "Utils.h"

using Poco::URI;


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

	URI uri(request().getURI());
	if (request().getMethod() == "GET") {
		_log.information(Poco::format("query: %s", uri.getQuery()));
		Poco::RegularExpression re("&");
		vector<string> pairs;
		int res = re.split(uri.getQuery(), pairs);
		_log.information(Poco::format("query x%d", res));
		for (vector<string>::iterator it = pairs.begin(); it != pairs.end(); it++) {
			_log.information(Poco::format("query[%s]", (*it)));
			string::size_type pos = (*it).find_first_of("=");
			if (pos != string::npos) {
				string name = (*it).substr(0, pos);
				string value = (*it).substr(pos + 1);
				_log.information(Poco::format("query[%s]=[%s]", name, value));
				if (!form().has(name)) {
					form().set(name, value);
				}
			}
		}
	}
	vector<string> urls;
	svvitch::split(uri.getPath().substr(1), '/', urls);
	int count = urls.size();
//	int count = re.split(request.getURI(), 0, urls);
	if (!urls.empty()) {
//		int num = 0;
//		Poco::NumberParser::tryParse(v[2], num);
		if (urls[0] == "remote") {
			remote();
		} else {
			response().setContentType("text/html; charset=UTF-8");
			sendErrorResponse(HTTPResponse::HTTP_NOT_FOUND, Poco::format("not found command: %s", urls[0]));
		}
	} else {
		response().setContentType("text/html; charset=UTF-8");
		sendErrorResponse(HTTPResponse::HTTP_NOT_FOUND, request().getURI());
	}
//	response().setChunkedTransferEncoding(true);
//	response().setContentType("text/plain; charset=UTF-8");

//	std::ostream& os = response().send();
//	os << Poco::format("%s split %d", request().getURI(), count);
//	os.flush();
}

void SwitchRequestHandler::remote() {
	MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer->getScene("main"));
	if (scene) {
		string playlistID = form().get("pl");
		if (!playlistID.empty()) {
			_log.information(Poco::format("playlist: %s", playlistID));
			ContainerPtr c = new Container(*_renderer);
			bool result = scene->prepareMedia(c, playlistID);
			response().setChunkedTransferEncoding(true);
			response().setContentType("text/plain; charset=UTF-8");
			if (result) {
				response().send() << Poco::format("%s", playlistID);
			} else {
				response().send() << Poco::format("failed prepared %s", playlistID);
			}
			SAFE_DELETE(c);
		} else {
			response().setContentType("text/html; charset=UTF-8");
			sendErrorResponse(HTTPResponse::HTTP_NOT_FOUND, "empty playlist ID");
		}
	} else {
			response().setContentType("text/html; charset=UTF-8");
		sendErrorResponse(HTTPResponse::HTTP_NOT_FOUND, "scene not found");
	}
}
