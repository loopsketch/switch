#pragma once

#include <Poco/Logger.h>
#include <Poco/Net/AbstractHTTPRequestHandler.h>
#include <Poco/Net/HTMLForm.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/MessageHeader.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/PartHandler.h>
#include <Poco/Path.h>

#include <iostream>
#include <map>

using std::map;
using std::string;
using Poco::Path;
using Poco::Net::HTMLForm;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::HTTPResponse;
using Poco::Net::MessageHeader;


/**
 * Partハンドラ
 */
class BasePartHandler: public Poco::Net::PartHandler {
private:
	Poco::Logger& _log;

public:
	BasePartHandler();
	virtual ~BasePartHandler();

	void handlePart(const MessageHeader& header, std::istream& stream);
};



/**
 * HTTPリクエストハンドラ
 */
class BaseRequestHandler: public HTTPRequestHandler {
private:
	HTTPServerRequest* _request;
	HTTPServerResponse* _response;
	HTMLForm* _form;

protected:
	Poco::Logger& _log;
	HTTPServerRequest& request();
	HTTPServerResponse& response();
	HTMLForm& form();

	virtual void doRequest();

	/** クライアントにファイルを送信します */
	bool sendFile(Path& path);

	/** mapをJSONP形式で送信します */
	void sendJSONP(const string& functionName, const map<string, string>& json);

	void writeResult(const int code, const string& description);

	void sendResponse(HTTPResponse::HTTPStatus status, const string& message);

public:
	BaseRequestHandler();

	virtual ~BaseRequestHandler();

	void handleRequest(HTTPServerRequest& requesr, HTTPServerResponse& response);
};
