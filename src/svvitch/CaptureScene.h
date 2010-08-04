#pragma once

#include "Scene.h"
//#include "ui/UserInterfaceManager.h"
#include "streams.h"
#include "Dvdmedia.h"
#include "DSVideoRenderer.h"


class CaptureScene: public Scene
{
private:
	Poco::FastMutex _lock;
	DWORD _frame;

	BOOL _useStageCapture;
	int _deviceNo;
	int _routePinNo;
	int _deviceW;
	int _deviceH;
	int _deviceFPS;
	int _flipMode;
	GUID _deviceVideoType;
	RECT _clip;

	int _previewX;
	int _previewY;
	int _previewW;
	int _previewH;

	IBaseFilter* _device;
	IGraphBuilder* _gb;
	ICaptureGraphBuilder2* _capture;
	DSVideoRendererPtr _vr;
	IMediaControl* _mc;

	LPDIRECT3DTEXTURE9 _cameraImage;
	LPDIRECT3DSURFACE9 _surface;
	LPBYTE _gray;


	/** フィルタを生成します */
	bool createFilter();
	/** フィルタを解放します */
	void releaseFilter();

	bool fetchDevice(REFCLSID clsidDeviceClass, int index, IBaseFilter** pBf, string& deviceName);

	bool routeCrossbar(IBaseFilter *pSrc, int no);

	int dumpFilter(IGraphBuilder* gb);

	const string getPinName(long lType);

	const string errorText(HRESULT hr);

public:
	CaptureScene(Renderer& renderer);

	virtual ~CaptureScene();

	virtual bool initialize();

	LPDIRECT3DTEXTURE9 getCameraImage();

	LPBYTE getGrayScale();

	virtual void process();

	virtual void draw1();

	virtual void draw2();
};

typedef CaptureScene* CaptureScenePtr;
