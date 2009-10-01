#pragma once

#include "Scene.h"
#include "ui/UserInterfaceManager.h"
#include "streams.h"
#include "Dvdmedia.h"
#include "DSVideoRenderer.h"


class CaptureScene: public Scene
{
private:
	DWORD _frame;

	int _deviceNo;
	int _routePinNo;
	int _deviceW;
	int _deviceH;
	int _deviceFPS;
	GUID _deviceVideoType;
	int _samples;

	IGraphBuilder* _gb;
	ICaptureGraphBuilder2* _capture;
	DSVideoRendererPtr _vr;
	IMediaControl* _mc;

	LPD3DXEFFECT _fx;
	LPDIRECT3DTEXTURE9 _cameraImage;
	vector<LPDIRECT3DTEXTURE9> _mavgTextures;

	bool fetchDevice(REFCLSID clsidDeviceClass, int index, IBaseFilter** pBf, string& deviceName);
	bool routeCrossbar(IBaseFilter *pSrc, int no);
	int dumpFilter(IGraphBuilder* gb);
	const string getPinName(long lType);
	const string errorText(HRESULT hr);

public:
	CaptureScene(Renderer& renderer, ui::UserInterfaceManagerPtr uim);

	~CaptureScene();

	virtual void initialize();

	LPDIRECT3DTEXTURE9 getCameraImage();

	virtual void process();

	virtual void draw1();

	virtual void draw2();

};

typedef CaptureScene* CaptureScenePtr;