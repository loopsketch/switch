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
 * �L���v�`���[�V�[���N���X.
 * DirectShow�L���v�`���[�\�[�X����f�����擾���� Scene �N���X�ł��B
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


	/** �ݒ肳�ꂽ�f�o�C�X����L���v�`���������s���t�B���^�O���t�𐶐����܂� */
	bool createFilter();

	/** �t�B���^�O���t��������܂� */
	void releaseFilter();

	/**
	 * �L���v�`���f�o�C�X�̃t�F�b�`���s���A�f�o�C�X�̃t�B���^��Ԃ��܂��B
	 * @param clsidDeviceClass	�N���XID
	 * @param index				�f�o�C�X�̃C���f�b�N�X�ԍ�
	 * @param pBf				�������ꂽ�f�o�C�X�̃t�B���^
	 * @param deviceName		�f�o�C�X���Ńt�F�b�`����ꍇ�Ɏw��
	 */
	bool fetchDevice(REFCLSID clsidDeviceClass, int index, IBaseFilter** pBf, string& deviceName = string());

	/**
	 * �t�B���^�̃s�����擾���܂�
	 * @param	filter	�Ώۂ̃t�B���^
	 * @param	pin		�擾�ł����s��
	 * @param	dir		�s������(IN/OUT)
	 */
	bool getPin(IBaseFilter* filter, IPin** pin, PIN_DIRECTION dir);

	/** �w�肵���t�B���^�̓��̓s����Ԃ��܂� */
	bool getInPin(IBaseFilter* filter, IPin** pin);

	/** �w�肵���t�B���^�̏o�̓s����Ԃ��܂� */
	bool getOutPin(IBaseFilter* filter, IPin** pin);

	/** �z���C�g�o�����X�̐ݒ� */
	void setWhiteBalance(IBaseFilter* src, bool autoFlag, long v = -100);

	/** �I�o�̐ݒ� */
	void setExposure(IBaseFilter* src, bool autoFlag, long v = -100);

	/** �N���X�o�[�����[�e�B���O���܂� */
	bool routeCrossbar(IBaseFilter *pSrc, int no);

	/** �t�B���^�̃_���v */
	int dumpFilter(IGraphBuilder* gb);

	/** �s������Ԃ��܂� */
	const string getPinName(long lType);

	/** �G���[�������Ԃ��܂� */
	const string errorText(HRESULT hr);

	/** �v���C���X�g��ύX���܂� */
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
