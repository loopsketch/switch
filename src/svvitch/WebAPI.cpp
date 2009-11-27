#include "WebAPI.h"

#include <Poco/format.h>
#include <Poco/RegularExpression.h>


SwitchRequestHandlerFactory::SwitchRequestHandlerFactory(RendererPtr renderer): _log(Poco::Logger::get("")), _renderer(renderer) {
}

SwitchRequestHandlerFactory::~SwitchRequestHandlerFactory() {
	_log.information("release SwitchRequestHandlerFactory");
}

HTTPRequestHandler* SwitchRequestHandlerFactory::createRequestHandler(const HTTPServerRequest& request) {
	return new SwitchRequestHandler(_renderer);
}


SwitchRequestHandler::SwitchRequestHandler(RendererPtr renderer): _log(Poco::Logger::get("")), _renderer(renderer) {
}

SwitchRequestHandler::~SwitchRequestHandler() {
	_log.information("release SwitchRequestHandler");
}

void SwitchRequestHandler::handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
	_log.information("request from " + request.clientAddress().toString());

	Poco::RegularExpression re("/");
	vector<string> v;
	int count = re.split(request.getURI(), 0, v);
//	svvitch::split(request.getURI(), '/', v);
	if (v.size() > 0) _log.information("url0: " + v[0]);
	if (v.size() > 1) _log.information("url1: " + v[1]);
	if (v.size() > 2) _log.information("url2: " + v[2]);
	if (v.size() >= 3) {
		int num = 0;
//		Poco::NumberParser::tryParse(v[2], num);
		if (v[1] == "update") {
//			return new UpdateRequestHandler(_scene, num);
		} else if (v[1] == "change") {
//			return new ChangeRequestHandler(_scene, num);
		} else if (v[1] == "check") {
//			return new CheckRequestHandler(_scene, num);
		}
	}

	// scene�ɒʒm
//	_scene.update(_num);

	response.setChunkedTransferEncoding(true);
	response.setContentType("text/html; charset=UTF-8");

	std::ostream& ostr = response.send();
	ostr << Poco::format("%s split %d", request.getURI(), count);
//	ostr.flush();
}
