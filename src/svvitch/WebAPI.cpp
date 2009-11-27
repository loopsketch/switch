#include "WebAPI.h"

#include <Poco/format.h>
#include <Poco/RegularExpression.h>
#include <Poco/Net/HTMLForm.h>

#include "MainScene.h"
#include "Utils.h"


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

	vector<string> urls;
	svvitch::split(request().getURI().substr(1), '/', urls);
	int count = urls.size();
//	Poco::RegularExpression re("/");
//	int count = re.split(request.getURI(), 0, urls);
	if (!urls.empty()) {
//		int num = 0;
//		Poco::NumberParser::tryParse(v[2], num);
		if (urls[0] == "remote") {
			remote();
		} else {
			sendErrorResponse(HTTPResponse::HTTP_NOT_FOUND, request().getURI());
		}
	} else {
		sendErrorResponse(HTTPResponse::HTTP_NOT_FOUND, request().getURI());
	}
//	response().setChunkedTransferEncoding(true);
//	response().setContentType("text/plain; charset=UTF-8");

//	std::ostream& os = response().send();
//	os << Poco::format("%s split %d", request().getURI(), count);
//	os.flush();
}

void SwitchRequestHandler::remote() {
	response().setChunkedTransferEncoding(true);
	response().setContentType("text/plain; charset=UTF-8");
	MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer->getScene("main"));
	if (scene) {
		string playlistID = form().get("pl");
		if (!playlistID.empty()) {
			_log.information(Poco::format("playlist: %s", playlistID));
			ContainerPtr c = new Container(*_renderer);
			bool result = scene->prepareMedia(c, playlistID);
			if (result) {
				response().send() << Poco::format("%s", playlistID);
			} else {
				response().send() << Poco::format("failed prepared %s", playlistID);
			}
			SAFE_DELETE(c);
		} else {
			sendErrorResponse(HTTPResponse::HTTP_NOT_FOUND, "empty playlist ID");
		}
	} else {
		sendErrorResponse(HTTPResponse::HTTP_NOT_FOUND, "scene not found");
	}
}
