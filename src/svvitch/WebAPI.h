#include <Poco/Logger.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/AbstractHTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/ServerSocket.h>

#include "Renderer.h"


using Poco::Net::AbstractHTTPRequestHandler;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::HTTPResponse;


/**
 * HTTPリクエストハンドラ
 */
class SwitchRequestHandler: public AbstractHTTPRequestHandler {
private:
	Poco::Logger& _log;
	RendererPtr _renderer;

	void get();
	void set();
	void svvitch();
	void writeResult(const int code, const string& description);

public:
	SwitchRequestHandler(RendererPtr renderer);

	virtual ~SwitchRequestHandler();

	void run();
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

	virtual ~SwitchRequestHandlerFactory();

	HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request);
};
