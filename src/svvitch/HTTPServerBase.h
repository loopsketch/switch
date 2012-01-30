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
 * �}���`�p�[�g�����N���X.
 * �t�H�[���̃}���`�p�[�g���������܂�
 */
class BasePartHandler: public Poco::Net::PartHandler {
private:
	Poco::Logger& _log;

public:
	/** �R���X�g���N�^ */
	BasePartHandler();
	/** �f�X�g���N�^ */
	virtual ~BasePartHandler();

	/**
	 * �p�[�g�̏������s���܂�.
	 * �R���e���c�̃p�[�g�ł���΁Auploads�f�B���N�g���ɏo�͂��܂��B
	 */
	void handlePart(const MessageHeader& header, std::istream& stream);
};



/**
 * HTTP���N�G�X�g�n���h���̊�{�N���X.
 */
class BaseRequestHandler: public HTTPRequestHandler {
private:
	HTTPServerRequest* _request;
	HTTPServerResponse* _response;
	HTMLForm* _form;

protected:
	Poco::Logger& _log;
	/** ���N�G�X�g */
	HTTPServerRequest& request();
	/** ���X�|���X */
	HTTPServerResponse& response();
	/** �t�H�[�� */
	HTMLForm& form();

	/**
	 * ���N�G�X�g����.
	 * handleRequest()����Ă΂�܂��̂ŃT�u�N���X�Ŏ������܂��B
	 */
	virtual void doRequest();

	/** �N���C�A���g�Ƀt�@�C���𑗐M���܂� */
	bool sendFile(Path& path);

	/** map��JSONP�`���ő��M���܂� */
	void sendJSONP(const string& functionName, const map<string, string>& json);

	/** ���ʂ��o�͂��܂� */
	void writeResult(const int code, const string& description);

	/** ���X�|���X�𑗐M���܂� */
	void sendResponse(HTTPResponse::HTTPStatus status, const string& message);

public:
	/** �R���X�g���N�^ */
	BaseRequestHandler();

	/** �f�X�g���N�^ */
	virtual ~BaseRequestHandler();

	/** ���N�G�X�g���� */
	void handleRequest(HTTPServerRequest& requesr, HTTPServerResponse& response);
};
