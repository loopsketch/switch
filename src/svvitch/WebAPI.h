#pragma once

#include "HTTPServerBase.h"
#include "Renderer.h"


/**
 * HTTPリクエストハンドラ
 */
class SwitchRequestHandler: public BaseRequestHandler {
private:
	Renderer& _renderer;

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

	/** ストックファイルをクリアします */
	void clearStock();

	/** コピー */
	void copy();

	/** バージョン */
	void version();


protected:
	void doRequest();


public:
	SwitchRequestHandler(Renderer& renderer);

	virtual ~SwitchRequestHandler();
};


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
