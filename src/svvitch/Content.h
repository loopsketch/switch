#pragma once

#include <string>
#include <vector>
#include <Poco/ActiveMethod.h>
#include <Poco/format.h>
#include <Poco/Logger.h>
#include <Poco/HashMap.h>

#include "Renderer.h"
#include "MediaItem.h"

using std::string;
using std::vector;
using std::wstring;
using Poco::HashMap;
using Poco::ActiveMethod;


/**
 * コンテントクラス.
 * <Scene>の中で描画される際の最小単位です。
 * 汎用的なコンポーネントとして利用したいものは<Content>にすべきです
 */
class Content
{
private:
	//static const string NULL_STRING;

protected:
	Poco::Logger& _log;
	Renderer& _renderer;

	int _splitType;
//	MediaItemPtr _media;
	string _mediaID;
	bool _playing;

	int _duration;
	int _current;

	int _keycode;
	bool _shift;
	bool _ctrl;

	HashMap<string, string> _properties;
	float _x, _y, _w, _h;

public:
	/** コンストラクタ */
	Content(Renderer& renderer, int splitType, float x = 0, float y = 0, float w = 0, float h = 0);

	/** デストラクタ */
	virtual ~Content();

	/** 初期化 */
	virtual void initialize();

	/** ファイルをオープンします */
	virtual bool open(const MediaItemPtr media, const int offset = 0);

	/** ファイルの準備が完了しているかどうか */
	virtual const string opened() const;

	/** 再生 */
	virtual void play();

	/** ポーズ */
	virtual void pause();

	/** 停止 */
	virtual void stop();

	/** 再生終了時にすぐさま停止するかどうか */
	virtual bool useFastStop();

	/** 頭だしする */
	virtual void rewind();

	/** 再生中かどうか */
	virtual const bool playing() const;

	/** 終了したかどうか */
	virtual const bool finished();

	/** クローズします */
	virtual void close();

	ActiveMethod<void, void, Content> activeClose;

	/**
	 * キー通知
	 * @param	keycide	押下キーコード
	 * @param	shift	SHIFTキーの押下フラグ
	 * @param	ctrl	CTRLキーの押下フラグ
	 */
	void notifyKey(const int keycode, const bool shift, const bool ctrl);

	/** 1フレームに1度だけ処理される */
	virtual void process(const DWORD& frame);

	/** 描画 */
	virtual void draw(const DWORD& frame);

	/** プレビュー描画 */
	virtual void preview(const DWORD& frame);

	/** 現在のフレーム */
	virtual const int current() const;

	/** 長さ(フレーム数) */
	virtual const int duration() const;

	/** 位置設定 */
	virtual void setPosition(float x, float y);

	/** 位置取得 */
	virtual void getPosition(float& x, float& y);

	/** 領域設定 */
	virtual void setBounds(float w, float h);

	/** 指定位置が領域範囲内かどうか */
	virtual const bool contains(float x, float y) const;

	/** パラメータを設定します */
	void set(const string& key, const string& value);

	/** パラメータを設定します */
	void set(const string& key, const float& value);

	/** パラメータを設定します */
	void set(const string& key, const unsigned int& value);

	/** パラメータを取得します */
	const string& get(const string& key, const string& defaultValue = "") const;

	/** パラメータを取得します */
	const DWORD getDW(const string& key, const DWORD& defaultValue = 0) const;

	/** パラメータを取得します */
	const int getI(const string& key, const int& defaultValue = 0) const;

	/** パラメータを取得します */
	const float getF(const string& key, const float& defaultValue = 0) const;
};

typedef Content* ContentPtr;
