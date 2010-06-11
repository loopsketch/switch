#include "HTTPServerBase.h"

#include <Poco/File.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/XML/XMLWriter.h>
#include <Poco/format.h>
#include <Poco/NumberFormatter.h>
#include <Poco/RegularExpression.h>
#include <Poco/Net/HTMLForm.h>
#include <Poco/FileStream.h>
#include <Poco/Path.h>
#include <Poco/StreamCopier.h>

#include "Common.h"
#include "Utils.h"

using Poco::File;
using Poco::RegularExpression;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Text;
using Poco::XML::AutoPtr;
using Poco::XML::DOMWriter;
using Poco::XML::XMLWriter;


BasePartHandler::BasePartHandler(): _log(Poco::Logger::get("")) {
	_log.debug("create BasePartHandler");
}

BasePartHandler::~BasePartHandler() {
	_log.debug("release BasePartHandler");
}

void BasePartHandler::handlePart(const MessageHeader& header, std::istream& is) {
	string type = header.get("Content-Type", "(unspecified)");
	if (header.has("Content-Disposition")) {
		string contentDisposition = header["Content-Disposition"]; // UTF-8ベースのページであればUTF-8になるようです
		string disp;
		Poco::Net::NameValueCollection params;
		MessageHeader::splitParameters(contentDisposition, disp, params);
		string name = params.get("name", "unnamed"); // formプロパティ名
		string fileName = params.get(name, "unnamed");
		_log.information(Poco::format("contentDisposition[%s] name[%s]", contentDisposition, name));
		Poco::RegularExpression re(".*filename=\"((.+\\\\)*(.+))\".*");
		if (re.match(contentDisposition)) {
			// IEのフルパス対策
			fileName = contentDisposition;
			re.subst(fileName, "$3");
			_log.information("fileName: " + fileName);
		}

		File uploadDir("uploads");
		if (!uploadDir.exists()) uploadDir.createDirectories();
		File f(uploadDir.path() + "/" + fileName + ".part");
		if (f.exists()) f.remove();
		try {
			Poco::FileOutputStream os(f.path());
			int size = Poco::StreamCopier::copyStream(is, os, 512 * 1024);
			os.close();
			File rename(uploadDir.path() + "/" + fileName);
			if (rename.exists()) rename.remove();
			f.renameTo(rename.path());
			_log.information(Poco::format("file %s %s %d", fileName, type, size));
		} catch (Poco::PathSyntaxException& ex) {
			_log.warning(Poco::format("failed download path[%s] %s", fileName, ex.displayText()));
		} catch (Poco::FileException& ex) {
			_log.warning(Poco::format("failed download file[%s] %s", fileName, ex.displayText()));
		}
	}
}



BaseRequestHandler::BaseRequestHandler():
	_log(Poco::Logger::get("")), _request(NULL), _response(NULL), _form(NULL) {
	_log.debug("create BaseRequestHandler");
}

BaseRequestHandler::~BaseRequestHandler() {
	SAFE_DELETE(_form);
	_log.debug("release BaseRequestHandler");
}

inline HTTPServerRequest& BaseRequestHandler::request() {
	return *_request;
}


inline HTTPServerResponse& BaseRequestHandler::response() {
	return *_response;
}

HTMLForm& BaseRequestHandler::form() {
	if (!_form) {
		BasePartHandler partHandler;
		_form = new HTMLForm(request(), request().stream(), partHandler);
	}
	return *_form;
}

void BaseRequestHandler::handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
	_request  = &request;
	_response = &response;

	_log.debug(Poco::format("contenttype %s", request.getContentType()));
	_log.debug(Poco::format("encoding %s", request.getTransferEncoding()));
	doRequest();
}

void BaseRequestHandler::doRequest() {
}

bool BaseRequestHandler::sendFile(Path& path) {
	try {
		Poco::FileInputStream is(path.toString());
		if (is.good()) {
			string ext = path.getExtension();
			File f(path);
			File::FileSize length = f.getSize();
			response().setContentLength(static_cast<int>(length));
			if (ext == "png") {
				response().setContentType("image/png");
			} else if (ext == "jpg" || ext == "jpeg") {
				response().setContentType("image/jpeg");
			} else if (ext == "bmp") {
				response().setContentType("image/bmp");
			} else if (ext == "gif") {
				response().setContentType("image/gif");
			} else if (ext == "mpg" || ext == "mpeg") {
				response().setContentType("video/mpeg");
			} else if (ext == "mp4" || ext == "f4v" || ext == "264") {
				response().setContentType("video/mp4");
			} else if (ext == "wmv") {
				response().setContentType("video/x-ms-wmv"); 
			} else if (ext == "mov") {
				response().setContentType("video/quicktime");
			} else if (ext == "flv") {
				response().setContentType("video/x-flv");
			} else if (ext == "swf") {
				response().setContentType("application/x-shockwave-flash");
			} else if (ext == "pdf") {
				response().setContentType("application/pdf");
			} else if (ext == "txt") {
				response().setContentType("text/plain");
			} else if (ext == "htm" || ext == "html") {
				response().setContentType("text/html");
			} else if (ext == "xml") {
				response().setContentType("text/xml");
			} else {
				response().setContentType("application/octet-stream");
			}
			response().setChunkedTransferEncoding(false);
			Poco::StreamCopier::copyStream(is, response().send(), 512 * 1024);
			is.close();
			return true;
		} else {
			throw Poco::OpenFileException(path.toString());
		}
	} catch (Poco::FileException& ex) {
		_log.warning(ex.displayText());
		//sendResponse(HTTPResponse::HTTP_NOT_FOUND, ex.displayText());
	}
	return false;
}

void BaseRequestHandler::sendJSONP(const string& functionName, const map<string, string>& json) {
	//response().setChunkedTransferEncoding(true);
	response().setContentType("text/javascript; charset=UTF-8");
	response().add("CacheControl", "no-cache");
	response().add("Expires", "-1");
	if (functionName.empty()) {
		// コールバック関数名が無い場合はJSONとして送信
		response().send() << svvitch::formatJSON(json);
	} else {
		response().send() << Poco::format("%s(%s);", functionName, svvitch::formatJSON(json));
	}
}

void BaseRequestHandler::writeResult(const int code, const string& description) {
	AutoPtr<Document> doc = new Document();
	AutoPtr<Element> remote = doc->createElement("remote");
	doc->appendChild(remote);
	AutoPtr<Element> result = doc->createElement("result");
	result->setAttribute("code", Poco::format("%d", code));
	AutoPtr<Text> resultText = doc->createTextNode(description);
	result->appendChild(resultText);
	remote->appendChild(result);
	DOMWriter writer;
	writer.setNewLine("\r\n");
	writer.setOptions(XMLWriter::WRITE_XML_DECLARATION | XMLWriter::PRETTY_PRINT);

	//response().setChunkedTransferEncoding(true);
	response().setContentType("text/xml; charset=UTF-8");
	writer.writeNode(response().send(), doc);
}

void BaseRequestHandler::sendResponse(HTTPResponse::HTTPStatus status, const string& message) {
	response().setStatusAndReason(status, message);

	string statusCode(Poco::NumberFormatter::format(static_cast<int>(response().getStatus())));
	//response().setChunkedTransferEncoding(true);
	if (message.find("<html>") == string::npos) {
		response().setContentType("text/plain; charset=UTF-8");
		if (status == HTTPResponse::HTTP_OK) {
			response().send() << message << std::endl;
		} else {
			response().send() << Poco::format("%s - %s", statusCode, message) << std::endl;
		}
	} else {
		response().setContentType("text/html; charset=UTF-8");
		response().send() << message;
	}
	response().send().flush();
}
