
#include <Poco/File.h>
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

#include "Renderer.h"

#include <iostream>
#include <map>

using std::map;
using std::string;
using Poco::File;
using Poco::Net::HTMLForm;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::HTTPResponse;
using Poco::Net::MessageHeader;

/**
 * Part�n���h��
 */
class SwitchPartHandler: public Poco::Net::PartHandler {
private:
	Poco::Logger& _log;
public:
	SwitchPartHandler();
	virtual ~SwitchPartHandler();

	void handlePart(const MessageHeader& header, std::istream& stream);
};


/**
 * HTTP���N�G�X�g�n���h��
 */
class SwitchRequestHandler: public HTTPRequestHandler {
private:
	Poco::Logger& _log;
	RendererPtr _renderer;

	HTTPServerRequest*  _request;
	HTTPServerResponse* _response;
	HTMLForm*           _form;

	HTTPServerRequest& request();
	HTTPServerResponse& response();
	HTMLForm& form();

	void doRequest();

	/** �ؑ� */
	void switchContent();

	/** �ݒ�n */
	void set(const string& name);

	/** �擾�n */
	void get(const string& name);

	/** �t�@�C���ꗗ */
	void files(const string& name);

	/** �t�@�C����JSON������ */
	string fileToJSON(const File f);

	/** �_�E�����[�h */
	void download(const string& name);

	/** �A�b�v���[�h */
	void upload(const string& name);

	/** map��JSONP�`���ő��M���܂� */
	void sendJSONP(const string& functionName, const map<string, string>& json);

	void writeResult(const int code, const string& description);

	void sendResponse(HTTPResponse::HTTPStatus status, const string& message);

public:
	SwitchRequestHandler(RendererPtr renderer);

	virtual ~SwitchRequestHandler();

	void handleRequest(HTTPServerRequest& requesr, HTTPServerResponse& response);
};

inline HTTPServerRequest& SwitchRequestHandler::request() {
	poco_check_ptr (_request);	
	return *_request;
}


inline HTTPServerResponse& SwitchRequestHandler::response() {
	poco_check_ptr (_response);
	return *_response;
}


/**
 * HTTP���N�G�X�g�n���h���p�t�@�N�g��
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
