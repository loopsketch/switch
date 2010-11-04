#pragma once

#include "Scene.h"
//#include "ui/UserInterfaceManager.h"
#include "streams.h"
#include "Dvdmedia.h"
#include "DSVideoRenderer.h"
#include <Poco/ActiveMethod.h>
#include "MainScene.h"


using Poco::ActiveMethod;


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
	bool _autoWhiteBalance;
	int _whiteBalance;
	bool _autoExposure;
	int _exposure;
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
	int _ignoreDetectTime;
	int _ignoreDetectCount;

	MainScenePtr _main;


	/** �t�B���^�𐶐����܂� */
	bool createFilter();
	/** �t�B���^��������܂� */
	void releaseFilter();

	bool fetchDevice(REFCLSID clsidDeviceClass, int index, IBaseFilter** pBf, string& deviceName);

	/** �z���C�g�o�����X�̐ݒ� */
	void setWhiteBalance(IBaseFilter* src, bool autoFlag, long v = -100);

	/** �I�o�̐ݒ� */
	void setExposure(IBaseFilter* src, bool autoFlag, long v = -100);

	/* �N���X�o�[�����[�e�B���O���܂� */
	bool routeCrossbar(IBaseFilter *pSrc, int no);

	int dumpFilter(IGraphBuilder* gb);

	const string getPinName(long lType);

	const string errorText(HRESULT hr);

	ActiveMethod<bool, void, CaptureScene> activeChangePlaylist;

	/** �v���C���X�g��ύX���܂� */
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
