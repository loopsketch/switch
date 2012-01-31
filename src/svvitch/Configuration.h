#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <Poco/Path.h>
#include <Poco/Channel.h>
#include <Poco/Logger.h>

using std::string;
using std::wstring;
using std::vector;
using Poco::Path;


/**
 * 設定管理クラス.
 * XMLから読込んだ値を保持します
 */
class Configuration
{
private:
	Poco::Logger& _log;

public:
	/** ログ出力用のChannel */
	Poco::Channel* logFile;

	/** windowタイトル */
	string windowTitle;

	/** ディスプレイ名 */
	string name;

	/** 説明 */
	string description;

	/** メイン画面領域 */
	RECT mainRect;

	/** メイン画面更新周期 */
	int mainRate;

	/** サブ画面領域 */
	RECT subRect;

	/** サブ画面更新周期 */
	int subRate;

	/** 画面更新単位のウェイト */
	DWORD frameIntervals;

	/** windowフレーム使用 */
	bool frame;

	/** フルスクリーンモードか */
	bool fullsceen;

	/** クリップ領域を使用するか */
	bool useClip;

	/** クリップ領域 */
	RECT clipRect;

	/** ステージ領域 */
	RECT stageRect;

	/** スナップショット用の品質(縮小率)設定 */
	float captureQuality;

	/** スナップショット用のフィルター設定 */
	string captureFilter;

	/** 分割タイプ */
	int splitType;

	/** 分割サイズ */
	SIZE splitSize;

	/** 分割繰返し回数 */
	int splitCycles;

	/** 利用動画再生エンジン */
	vector<string> movieEngines;

	/** 利用シーン名 */
	vector<string> scenes;

	/** ステージの輝度(0-100) */
	int brightness;

	/** ディマー(0-1.0) */
	float dimmer;

	/** 画像の分割幅px */
	bool viewStatus;

	/** 画像の分割幅px */
	int imageSplitWidth;

	/** テキストのフォント */
	string textFont;

	/** テキストのスタイル */
	string textStyle;

	/** テキストの高さ */
	int textHeight;

	/** マウスのカーソル表示可否 */
	bool mouse;

	/** windowのドラック可否 */
	bool draggable;

	/** フォントファイル */
	wstring defaultFont;

	/** ASCIIフォントファイル */
	string asciiFont;

	/** フォントファイル */
	string multiByteFont;
	// string vpCommandFile;
	// string monitorFile;

	/** データディレクトリルート */
	Path dataRoot;

	/** 送信ファイルの一時ストック場所 */
	Path stockRoot;

	/** ワークスペース設置パス */
	Path workspaceFile;

	/** ニュースURL */
	string newsURL;

	/** サーバポート */
	int serverPort;

	/** サーバキュー数 */
	int maxQueued;

	/** サーバスレッド数 */
	int maxThreads;

	/** 送出ログを出力するか */
	bool outCastLog;



	/** コンストラクタ */
	Configuration();

	/** デストラクタ */
	virtual ~Configuration();

	/** 初期化 */
	bool initialize();

	/** 保存 */
	void save();

	/** 指定したシーン名が設定に書いてあるかどうか */
	bool hasScene(string s);

	/** 開放 */
	void release();
};

typedef Configuration* ConfigurationPtr;