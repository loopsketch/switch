#pragma once

#include <Poco/ActiveMethod.h>
#include <Poco/ActiveResult.h>
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>

#include "Content.h"
#include "CaptureScene.h"
#include "MediaItem.h"
#include "FFMovieContent.h"
#include "PerformanceTimer.h"
#include "Renderer.h"
#include "Workspace.h"

#include <queue>

using std::queue;
using std::string;
using std::wstring;


/**
 * OpenCV利用コンテントクラス.
 * OpenCVの実験コンテントです
 */
class CvContent: public Content
{
private:
	Poco::FastMutex _lock;

	CaptureScenePtr _scene;

	float _subtract;
	int _intervalDiff;
	int _intervalSmall;

	int _clipX;
	int _clipY;
	int _clipW;
	int _clipH;

	LPD3DXEFFECT _fx;
	LPDIRECT3DTEXTURE9 _small1;
	LPDIRECT3DTEXTURE9 _small2;
	LPDIRECT3DTEXTURE9 _diff;
	LPDIRECT3DSURFACE9 _diff2;
	LPDIRECT3DTEXTURE9 _photo;
	int _detectThreshold;
	int _diffCount;

	string _normalFile;
	MediaItemPtr _normalItem;
	FFMovieContentPtr _normalMovie;
	vector<string> _detectFiles;
	MediaItemPtr _detectedItem;
	FFMovieContentPtr _detectedMovie;

	int _detectCount;
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
	CvContent(Renderer& renderer, int splitType);

	/** デストラクタ */
	virtual ~CvContent();

	/** 設定の保存 */
	void saveConfiguration();

	/** 初期化 */
	void initialize();

	/** ファイルをオープンします */
	bool open(const MediaItemPtr media, const int offset = 0);

	Poco::ActiveMethod<void, void, CvContent> activeOpenDetectMovie;
	void openDetectMovie();

	/** 再生 */
	void play();

	/** 停止 */
	void stop();

	/** 再生終了時にすぐさま停止するかどうか */
	bool useFastStop();

	/** 再生中かどうか  */
	const bool playing() const;

	const bool finished();

	/** ファイルをクローズします */
	void close();

	/** 描画以外の処理 */
	void process(const DWORD& frame);

	/** 描画処理 */
	void draw(const DWORD& frame);
};

typedef CvContent* CvContentPtr;
