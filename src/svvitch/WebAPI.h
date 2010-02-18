#pragma once

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
#include <Poco/Path.h>

#include "Renderer.h"

#include <iostream>
#include <map>

using std::map;
using std::string;
using Poco::File;
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
class SwitchPartHandler: public Poco::Net::PartHandler {
private:
	Poco::Logger& _log;
public:
	SwitchPartHandler();
	virtual ~SwitchPartHandler();

	void handlePart(const MessageHeader& header, std::istream& stream);
};


/**
 * HTTPリクエストハンドラ
 */
class SwitchRequestHandler: public HTTPRequestHandler {
private:
	Poco::Logger& _log;
	Renderer& _renderer;

	HTTPServerRequest*  _request;
	HTTPServerResponse* _response;
	HTMLForm*           _form;

	HTTPServerRequest& request();
	HTTPServerResponse& response();
	HTMLForm& form();

	void doRequest();

	/** 切替 */
	void switchContent();

	/** 更新 */
	void updateWorkspace();

	/** 設定系 */
	void set(const string& name);

	/** 取得系 */
	void get(const string& name);

	/**
	 * ファイル情報を返します.
	 * ファイルのパスを指定した場合はファイル情報をmapで.
	 * ディレクトリを指定した場合は子のファイル名&ディレクトリ名の配列で.
	 */
	void files();

	/** ファイルをJSON化する */
	string fileToJSON(const Path path);

	/** ダウンロード */
	void download();

	/** アップロード */
	void upload();

	/** コピー */
	void copy();

	/** mapをJSONP形式で送信します */
	void sendJSONP(const string& functionName, const map<string, string>& json);

	void writeResult(const int code, const string& description);

	void sendResponse(HTTPResponse::HTTPStatus status, const string& message);

public:
	SwitchRequestHandler(Renderer& renderer);

	virtual ~SwitchRequestHandler();

	void handleRequest(HTTPServerRequest& requesr, HTTPServerResponse& response);
};

inline HTTPServerRequest& SwitchRequestHandler::request() {
//	poco_check_ptr(_request);	
	return *_request;
}


inline HTTPServerResponse& SwitchRequestHandler::response() {
//	poco_check_ptr(_response);
	return *_response;
}


/**
 * HTTPリクエストハンドラ用ファクトリ
 */
class SwitchRequestHandlerFactory: public HTTPRequestHandlerFactory {	
private:
	Poco::Logger& _log;
	Renderer& _renderer;

public:
	SwitchRequestHandlerFactory(Renderer& renderer);

	virtual ~SwitchRequestHandlerFactory();

	HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request);
};
