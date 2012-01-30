#pragma once

#include <queue>
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>

#include "Content.h"
#include "CaptureScene.h"
#include "MediaItem.h"
#include "PerformanceTimer.h"
#include "Renderer.h"
#include "Workspace.h"

using std::queue;
using std::string;
using std::wstring;


/**
 * キャプチャーコンテントクラス.
 * CaptureScene からキャプチャー映像を取得し描画する Content クラスです。
 */
class CaptureContent: public Content
{
private:
	Poco::FastMutex _lock;

	CaptureScenePtr _scene;

	float _subtract;
	int _intervalDiff;
	int _intervalSmall;

	LPD3DXEFFECT _fx;
	LPDIRECT3DTEXTURE9 _small1;
	LPDIRECT3DTEXTURE9 _small2;
	LPDIRECT3DTEXTURE9 _diff;
	LPDIRECT3DSURFACE9 _diff2;
	LPDIRECT3DTEXTURE9 _image;
	int _detectThreshold;
	int _diffCount;

	bool _detected;
	int _doShutter;
	int _viewPhoto;

	bool _finished;
	bool _playing;
	PerformanceTimer _playTimer;

	DWORD _statusFrame;
	string _status;

public:
	/** コンストラクタ */
	CaptureContent(Renderer& renderer, int splitType);

	/** デストラクタ */
	~CaptureContent();


	/** 設定を保存します */
	void saveConfiguration();

	/** 初期化 */
	void initialize();

	/** ファイルをオープンします */
	bool open(const MediaItemPtr media, const int offset = 0);


	/** 再生 */
	void play();

	/** 停止 */
	void stop();

	/** 再生終了時にすぐさま停止するかどうか */
	bool useFastStop();

	/** 再生中かどうか */
	const bool playing() const;

	/** 終了したかどうか */
	const bool finished();

	/** クローズします */
	void close();

	/** 描画以外の処理 */
	void process(const DWORD& frame);

	/** 描画処理 */
	void draw(const DWORD& frame);
};

typedef CaptureContent* CaptureContentPtr;