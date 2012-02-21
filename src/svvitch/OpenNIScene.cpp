#ifdef USE_OPENNI
#include "OpenNIScene.h"

#include <exception>

OpenNIScenePtr _openNIScene;

void XN_CALLBACK_TYPE callback_newUser(xn::UserGenerator& generator, XnUserID id, void* cookie) {
	_openNIScene->newUser(generator, id, cookie);
}

void XN_CALLBACK_TYPE callback_lostUser(xn::UserGenerator& generator, XnUserID id, void* cookie) {
	_openNIScene->lostUser(generator, id, cookie);
}

void XN_CALLBACK_TYPE callback_detectedPose(xn::PoseDetectionCapability& capability, const XnChar* pose, XnUserID id, void* cookie) {
	_openNIScene->detectedPose(capability, pose, id, cookie);
}

void XN_CALLBACK_TYPE callback_startCalibration(xn::SkeletonCapability& capability, XnUserID id, void* cookie) {
	_openNIScene->startCalibration(capability, id, cookie);
}

void XN_CALLBACK_TYPE callback_endCalibration(xn::SkeletonCapability& capability, XnUserID id, XnBool success, void* cookie) {
	_openNIScene->endCalibration(capability, id, success, cookie);
}


OpenNIScene::OpenNIScene(Renderer& renderer): Scene(renderer), _worker(NULL), _frame(0), _texture1(NULL), _texture2(NULL), _readCount(0), _avgTime(0)
{
	_openNIScene = this;
	_log.information("OpenNI-scene");
}

OpenNIScene::~OpenNIScene() {
	if (_worker) {
		_worker = NULL;
		_thread.join();
	}
	SAFE_RELEASE(_imageTexture);
	SAFE_RELEASE(_imageSurface);
	SAFE_RELEASE(_texture1);
	SAFE_RELEASE(_texture2);

	_log.information("*uninitialize OpenNI-scene");
}

bool OpenNIScene::initialize() {
	_imageTexture = NULL;
	_imageSurface = NULL;

	XnStatus ret = XN_STATUS_OK;
	//ret = _context.InitFromXmlFile("openni-config.xml", NULL);
	ret = _context.Init();
	if (ret != XN_STATUS_OK) {
		_log.warning("failed not initialize kinect");
		return false;
	}
	ret = _imageGenerator.Create(_context);
	//ret = _context.FindExistingNode(XN_NODE_TYPE_IMAGE, _image);
	if (ret != XN_STATUS_OK) {
		_log.warning("failed not found ImageGenerator");
		return false;
	}
	XnMapOutputMode mode;
	mode.nXRes = SENSOR_WIDTH;
	mode.nYRes = SENSOR_HEIGHT;
	mode.nFPS = 30;
	_imageGenerator.SetMapOutputMode(mode);
	_imageGenerator.SetPixelFormat(XN_PIXEL_FORMAT_RGB24);

	ret = _depthGenerator.Create(_context);
	//ret = _context.FindExistingNode(XN_NODE_TYPE_DEPTH, _depth);
	if (ret != XN_STATUS_OK) {
		_log.warning("failed not found DepthGenerator");
		return false;
	}
	_depthGenerator.SetMapOutputMode(mode);

	ret = _userGenerator.Create(_context);
	if (ret != XN_STATUS_OK) {
		_log.warning("failed not found UserGenerator");
		return false;
	}

	XnCallbackHandle userCallbacks;
	_userGenerator.RegisterUserCallbacks(callback_newUser, callback_lostUser, NULL, userCallbacks);
	if (_userGenerator.GetSkeletonCap().NeedPoseForCalibration()) {
		XnCallbackHandle poseCallbacks;
		_userGenerator.GetPoseDetectionCap().RegisterToPoseCallbacks(callback_detectedPose, NULL, NULL, poseCallbacks);
	}
	XnCallbackHandle calibrationCallbacks;
	_userGenerator.GetSkeletonCap().RegisterCalibrationCallbacks(callback_startCalibration, callback_endCalibration, NULL, calibrationCallbacks);
	_userGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);
	//_user.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_HEAD_HANDS); // head & hand
	
	_userGenerator.GetSkeletonCap().GetCalibrationPose(_pose);

	ret = _context.StartGeneratingAll();
	if (ret != XN_STATUS_OK) {
		_log.warning("failed not start GeneratingAll");
		return false;
	}

	_imageTexture = _renderer.createRenderTarget(SENSOR_WIDTH, SENSOR_HEIGHT, D3DFMT_A8R8G8B8);
	_imageSurface = _renderer.createLockableSurface(SENSOR_WIDTH, SENSOR_HEIGHT, D3DFMT_A8R8G8B8);
	_texture1 = _renderer.createRenderTarget(SENSOR_WIDTH, SENSOR_HEIGHT, D3DFMT_A8R8G8B8);
	_texture2 = _renderer.createRenderTarget(SENSOR_WIDTH, SENSOR_HEIGHT, D3DFMT_A8R8G8B8);

	_worker = this;
	_thread.start(*_worker);

	_log.information("*initialize OpenNI-scene");
	return true;
}

void OpenNIScene::newUser(xn::UserGenerator& user, XnUserID id, void* cookie) {
	XnStatus ret = XN_STATUS_OK;
	_log.information(Poco::format("new user %u", id));
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		_users[id] = new UserViewer(_renderer, _userGenerator, _depthGenerator, id);
	}
	xn::SkeletonCapability& skeleton = user.GetSkeletonCap();
	if (skeleton.NeedPoseForCalibration()) {
		user.GetPoseDetectionCap().StartPoseDetection(_pose, id);
	} else {
		skeleton.RequestCalibration(id, true);
	}
}

void OpenNIScene::lostUser(xn::UserGenerator& user, XnUserID id, void* cookie) {
	_log.information(Poco::format("lost user %u", id));
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		map<XnUserID, UserViewerPtr>::iterator i = _users.find(id);
		if (i != _users.end()) {
			_users.erase(id);
		}
	}
}

void OpenNIScene::detectedPose(xn::PoseDetectionCapability& poseDetection, const XnChar* strPose, XnUserID id, void* cookie) {
	_log.information("detected pose");
	poseDetection.StopPoseDetection(id);
	_userGenerator.GetSkeletonCap().RequestCalibration(id, true);
}

void OpenNIScene::startCalibration(xn::SkeletonCapability& skeleton, XnUserID id, void* cookie) {
	_log.information("start calibration");
}

void OpenNIScene::endCalibration(xn::SkeletonCapability& skeleton, XnUserID id, XnBool success, void* cookie) {
	if (success) {
		skeleton.StartTracking(id);
		_log.information("start tracking");
	} else {
		if (skeleton.NeedPoseForCalibration()) {
			_userGenerator.GetPoseDetectionCap().StartPoseDetection(_pose, id);
			_log.information("retry pose detection");
		} else {
			skeleton.RequestCalibration(id, true);
			_log.information("retry calibration");
		}
	}
}

void OpenNIScene::run() {
	SetThreadAffinityMask(::GetCurrentThread(), 1 | 2 | 4 | 8);
	PerformanceTimer timer;
	_readCount = 0;
	_avgTime = 0;
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		_renderer.colorFill(_imageTexture, 0xff000000);
	}

	_imageGenerator.GetMirrorCap().SetMirror(true);
	_depthGenerator.GetMirrorCap().SetMirror(true);
	xn::AlternativeViewPointCapability viewPoint = _depthGenerator.GetAlternativeViewPointCap();
	viewPoint.SetViewPoint(_imageGenerator);
	//viewPoint.ResetViewPoint();

	while (_worker) {
		try {
			timer.start();
			_context.WaitAndUpdateAll();
			_imageGenerator.GetMetaData(_imageMD);
			_depthGenerator.GetMetaData(_depthMD);
			_userGenerator.GetUserPixels(0, _sceneMD);
			_readTime = timer.getTime();
			_readCount++;
			if (_readCount > 0) _avgTime = F(_avgTime * (_readCount - 1) + _readTime) / _readCount;

			{
				Poco::ScopedLock<Poco::FastMutex> lock(_lock);
				for (map<XnUserID, UserViewerPtr>::iterator it = _users.begin(); it != _users.end(); it++) {
					it->second->process();
				}
			}
			D3DLOCKED_RECT lockedRect = {0};
			if SUCCEEDED(_imageSurface->LockRect(&lockedRect, NULL, 0)) {
				LPBYTE src = (LPBYTE)_imageMD.RGB24Data();
				LPUINT dst = (LPUINT)lockedRect.pBits;
				int pitchAdd = lockedRect.Pitch / 4 - SENSOR_WIDTH;
				int i = 0;
				int j = 0;
				UINT d, b, g, r;
				XnLabel label;
				for (int y = 0; y < SENSOR_HEIGHT; ++y) {
					for (int x = 0; x < SENSOR_WIDTH; ++x) {
						label = _sceneMD(x, y);
						//if (label > 0) {
						//	map<XnUserID, UserViewerPtr>::iterator it = _users.find((XnUserID)label);
						//	if (it != _users.end()) it->second->setHeight(y);
						//}
						d = 255 - ((256 * (_depthMD(x, y) - DEPTH_RANGE_MIN) >> 13)) & 0xff; // ëÂëÃd / DEPTH_RANGE_MAXÇçsÇ§ÇΩÇﬂÅA / 8192
						r = src[i++] << 16;
						g = src[i++] << 8;
						b = src[i++];
						switch (label) {
						case 1:
							dst[j++] = d << 24 | 0x0099ff;
							break;
						case 2:
							dst[j++] = d << 24 | 0xffcc00;
							break;
						case 3:
							dst[j++] = d << 24 | 0xff0099;
							break;
						case 4:
							dst[j++] = d << 24 | 0x009900;
							break;
						case 5:
							dst[j++] = d << 24 | 0x00ff99;
							break;
						default:
							dst[j++] = 0xff000000 | r | g | b;
							//dst[j++] = 0x00000000 | d;
						}
					}
					j+=pitchAdd;
				}
				_imageSurface->UnlockRect();
				if (!_renderer.updateRenderTargetData(_imageTexture, _imageSurface)) {
					_log.warning("updateRenderTargetData");
				}
			}
			{
				Poco::ScopedLock<Poco::FastMutex> lock(_lock);
				_renderer.copyTexture(_imageTexture, _texture1);
			}
			_fpsCounter.count();
		} catch (const std::exception& ex) {
			_log.warning(Poco::format("exception: %s", ex.what()));
		}
		//Poco::Thread::sleep(1);
	}
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		for (map<XnUserID, UserViewerPtr>::iterator it = _users.begin(); it != _users.end(); it++) {
			SAFE_DELETE(it->second);
		}
	}
	_context.StopGeneratingAll();
	_context.Release();
}

void OpenNIScene::process() {
	_frame++;
	//if ((_frame % 60) == 0) {
	//	LPDIRECT3DTEXTURE9 texture = _renderer.getCaptureTexture();
	//	if (texture) {
	//		if SUCCEEDED(D3DXSaveTextureToFileA(Poco::format("caps/capture_%05lu.png", _frame / 60).c_str(), D3DXIFF_PNG, texture, NULL)) {
	//		}
	//	}
	//}
}

void OpenNIScene::draw1() {
	if (_worker) {
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		LPDIRECT3DSURFACE9 target = NULL;
		HRESULT hr = device->GetRenderTarget(0, &target);
		LPDIRECT3DSURFACE9 surface = NULL;
		_texture2->GetSurfaceLevel(0, &surface);
		hr = device->SetRenderTarget(0, surface);
		if (_texture1) {
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			DWORD col = 0xffffffff;
			_renderer.drawTexture(0, 0, SENSOR_WIDTH, SENSOR_HEIGHT, _texture1, 0, col, col, col, col);
			for (map<XnUserID, UserViewerPtr>::iterator it = _users.begin(); it != _users.end(); it++) {
				it->second->draw();
			}
		}
		hr = device->SetRenderTarget(0, target);
		float z = F(config().stageRect.right) / SENSOR_WIDTH;
		float sw = SENSOR_WIDTH * z;
		float sh = SENSOR_HEIGHT * z;
		float sx = (config().stageRect.right - sw) / 2;
		float sy = (config().stageRect.bottom - sh) / 2;

		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		DWORD col = 0xffffffff;
		_renderer.drawTexture(config().stageRect.left, config().stageRect.top, config().stageRect.right, config().stageRect.bottom, sx, sy, sw, sh, _texture2, 0, col, col, col, col);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	}
}

void OpenNIScene::draw2() {
	if (_worker) {
		string s = Poco::format("%02lufps-%03.2hfms user-%?u", _fpsCounter.getFPS(), _avgTime, _users.size());
		_renderer.drawFontTextureText(0, 0, 10, 10, 0xccff3333, s);
	}
}


#endif
