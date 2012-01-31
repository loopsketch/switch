#pragma once

#include "Content.h"
#include <Poco/Logger.h>
#include <Poco/Mutex.h>
#include <vector>

using std::vector;


/**
 * 複数のContentを含むContentのコンテナ
 */
class Container: public Content
{
private:
	Poco::FastMutex _lock;
	vector<ContentPtr> _list;

	bool _initialized;

public:
	/** コンストラクタ */
	Container(Renderer& renderer);

	/** デストラクタ */
	virtual ~Container();

	/** 初期化 */
	void initialize();

	/**
	 * コンテナにContentを追加します
	 * @param c	追加するコンテンツ
	 */
	void add(ContentPtr c);

	/** 配列でアクセス */
	ContentPtr operator[](int i);

	/**
	 * 指定したインデックスのコンテンツを返します
	 * @param i	インデックス番号
	 */
	ContentPtr get(int i);

	/** コンテンツ数 */
	int size();

	/**
	 * open済かどうか
	 * @return	オープンしているコンテンツID
	 */
	const string opened();

	/** 再生 */
	void play();

	/** ポーズ */
	void pause();

	/** 停止 */
	void stop();

	/** 停止時にすぐさま停止するかどうか */
	bool useFastStop();

	/** 頭出し */
	void rewind();

	/**
	 * 終了したかどうか
	 * @return	終了していればtrue、まだならfalse
	 */
	const bool finished();

	/**
	 * キー通知
	 * @param	keycide	押下キーコード
	 * @param	shift	SHIFTキーの押下フラグ
	 * @param	ctrl	CTRLキーの押下フラグ
	 */
	void notifyKey(const int keycode, const bool shift, const bool ctrl);

	/** 描画以外の処理 */
	void process(const DWORD& frame);

	/** 描画処理 */
	void draw(const DWORD& frame);

	/** プレビュー処理 */
	void preview(const DWORD& frame);

	/** 現在の再生フレーム */
	const int current();

	/** フレーム数 */
	const int duration();

	/** プロパティの設定 */
	void setProperty(const string& key, const string& value);
};

typedef Container* ContainerPtr;