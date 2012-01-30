#pragma once

#include "Scene.h"
//#include "ui/UserInterfaceManager.h"
#include "streams.h"
#include "Dvdmedia.h"
#include "DSVideoRenderer.h"
#include <Poco/ActiveMethod.h>
#include "MainScene.h"


using Poco::ActiveMethod;


/**
 * キャプチャーシーンクラス.
 * DirectShowキャプチャーソースから映像を取得する Scene クラスです。
 */
class CaptureScene: public Scene
{
private:
	Poco::FastMutex _lock;
	DWORD _frame;
	DWORD _startup;

	BOOL _useStageCapture;
	int _deviceNo;
	int _routePinNo;
	int _deviceW;
	int _deviceH;
	int _deviceFPS;
	bool _useDeinterlace;
	bool _autoWhiteBalance;
	int _whiteBalance;
	bool _autoExposure;
	int _exposure;
	int _flipMode;
	GUID _deviceVideoType;
	RECT _clip;

	int _px;
	int _py;
	int _pw;
	int _ph;

	int _spx;
	int _spy;
	int _spw;
	int _sph;

	IBaseFilter* _device;
	IGraphBuilder* _gb;
	ICaptureGraphBuilder2* _capture;
	DSVideoRendererPtr _vr;
	IMediaControl* _mc;

	LPDIRECT3DTEXTURE9 _cameraImage;
	LPDIRECT3DTEXTURE9 _sample;
	LPDIRECT3DSURFACE9 _surface;
	LPD3DXEFFECT _fx;

	int _sw;
	int _sh;
	LPINT _data1;
	LPINT _data2;
	LPINT _data3;
	bool _forceUpdate;
	int _intervalsBackground;
	int _intervalsForeground;
	LPBOOL _lookup;
	LPINT _block;
	LPBOOL _activeBlock;
	int _blockThreshold;
	int _lookupThreshold;
	int _detectCount;
	int _detectThreshold;
	string _detectedPlaylist;
	vector<string> _activePlaylist;
	int _ignoreDetectTime;
	int _ignoreDetectCount;

	MainScenePtr _main;


	/** 設定されたデバイスからキャプチャ処理を行うフィルタグラフを生成します */
	bool createFilter();

	/** フィルタグラフを解放します */
	void releaseFilter();

	/**
	 * キャプチャデバイスのフェッチを行い、デバイスのフィルタを返します。
	 * @param clsidDeviceClass	クラスID
	 * @param index				デバイスのインデックス番号
	 * @param pBf				生成されたデバイスのフィルタ
	 * @param deviceName		デバイス名でフェッチする場合に指定
	 */
	bool fetchDevice(REFCLSID clsidDeviceClass, int index, IBaseFilter** pBf, string& deviceName = string());

	/**
	 * フィルタのピンを取得します
	 * @param	filter	対象のフィルタ
	 * @param	pin		取得できたピン
	 * @param	dir		ピン方向(IN/OUT)
	 */
	bool getPin(IBaseFilter* filter, IPin** pin, PIN_DIRECTION dir);

	/** 指定したフィルタの入力ピンを返します */
	bool getInPin(IBaseFilter* filter, IPin** pin);

	/** 指定したフィルタの出力ピンを返します */
	bool getOutPin(IBaseFilter* filter, IPin** pin);

	/** ホワイトバランスの設定 */
	void setWhiteBalance(IBaseFilter* src, bool autoFlag, long v = -100);

	/** 露出の設定 */
	void setExposure(IBaseFilter* src, bool autoFlag, long v = -100);

	/** クロスバーをルーティングします */
	bool routeCrossbar(IBaseFilter *pSrc, int no);

	/** フィルタのダンプ */
	int dumpFilter(IGraphBuilder* gb);

	/** ピン名を返します */
	const string getPinName(long lType);

	/** エラー文字列を返します */
	const string errorText(HRESULT hr);

	/** プレイリストを変更します */
	ActiveMethod<bool, void, CaptureScene> activeChangePlaylist;
	bool changePlaylist();

public:
	CaptureScene(Renderer& renderer);

	virtual ~CaptureScene();

	virtual bool initialize();

	LPDIRECT3DTEXTURE9 getCameraImage();

	virtual void process();

	virtual void draw1();

	virtual void draw2();
};

typedef CaptureScene* CaptureScenePtr;
