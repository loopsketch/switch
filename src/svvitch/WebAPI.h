#include <Poco/Logger.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/ServerSocket.h>

#include "Renderer.h"


using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;


/**
 * HTTPリクエストハンドラ
 */
class SwitchRequestHandler: public HTTPRequestHandler {
private:
	Poco::Logger& _log;
	RendererPtr _renderer;

public:
	SwitchRequestHandler(RendererPtr renderer);

	~SwitchRequestHandler();

	void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response);
};


/**
 * HTTPリクエストハンドラ用ファクトリ
 */
class SwitchRequestHandlerFactory: public HTTPRequestHandlerFactory {	
private:
	Poco::Logger& _log;
	RendererPtr _renderer;

public:
	SwitchRequestHandlerFactory(RendererPtr renderer);

	~SwitchRequestHandlerFactory();

	HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request);
};
