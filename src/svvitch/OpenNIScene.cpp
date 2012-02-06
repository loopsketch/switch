#ifdef USE_OPENNI
#include "OpenNIScene.h"

OpenNIScene::OpenNIScene(Renderer& renderer): Scene(renderer)
{
	_log.information("OpenNI-scene");
}

OpenNIScene::~OpenNIScene() {
	if (_worker) {
		_worker = NULL;
		_thread.join();
	}
	_context.StopGeneratingAll();
	_context.Release();
	SAFE_RELEASE(_imageTexture);
	SAFE_RELEASE(_imageSurface);
	SAFE_RELEASE(_texture);

	_log.information("*uninitialize OpenNI-scene");
}

bool OpenNIScene::initialize() {
	_imageTexture = NULL;
	_imageSurface = NULL;
	_texture = NULL;

	XnStatus ret = XN_STATUS_OK;
	//ret = _context.InitFromXmlFile("openni-config.xml", NULL);
	ret = _context.Init();
	if (ret != XN_STATUS_OK) {
		_log.warning("failed not initialize kinect");
		return false;
	}
	ret = _image.Create(_context);
	//ret = _context.FindExistingNode(XN_NODE_TYPE_IMAGE, _image);
	if (ret != XN_STATUS_OK) {
		_log.warning("failed not found ImageGenerator");
		return false;
	}
	XnMapOutputMode mode;
	mode.nXRes = SENSOR_WIDTH;
	mode.nYRes = SENSOR_HEIGHT;
	mode.nFPS = 30;
	_image.SetMapOutputMode(mode);
	_image.SetPixelFormat(XN_PIXEL_FORMAT_RGB24);

	ret = _depth.Create(_context);
	//ret = _context.FindExistingNode(XN_NODE_TYPE_DEPTH, _depth);
	if (ret != XN_STATUS_OK) {
		_log.warning("failed not found DepthGenerator");
		return false;
	}
	_depth.SetMapOutputMode(mode);

	ret = _user.Create(_context);
	if (ret != XN_STATUS_OK) {
		_log.warning("failed not found UserGenerator");
		return false;
	}
	XnCallbackHandle userCallbacks;
	_user.RegisterUserCallbacks(newUser, lostUser, NULL, userCallbacks);
	XnCallbackHandle poseCallbacks;
	_user.GetPoseDetectionCap().RegisterToPoseCallbacks(detectedPose, NULL, NULL, poseCallbacks);
	XnCallbackHandle calibrationCallbacks;
	_user.GetSkeletonCap().RegisterCalibrationCallbacks(startCalibration, endCalibration, NULL, calibrationCallbacks);
	_user.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);
	_user.GetSkeletonCap().GetCalibrationPose(_pose);

	ret = _context.StartGeneratingAll();
	if (ret != XN_STATUS_OK) {
		_log.warning("failed not start GeneratingAll");
		return false;
	}

	_imageTexture = _renderer.createRenderTarget(SENSOR_WIDTH, SENSOR_HEIGHT, D3DFMT_A8R8G8B8);
	_imageSurface = _renderer.createLockableSurface(SENSOR_WIDTH, SENSOR_HEIGHT, D3DFMT_A8R8G8B8);
	_texture = _renderer.createRenderTarget(SENSOR_WIDTH, SENSOR_HEIGHT, D3DFMT_A8R8G8B8);

	_worker = this;
	_thread.start(*_worker);

	_log.information("*initialize OpenNI-scene");
	return true;
}

void OpenNIScene::run() {
	PerformanceTimer timer;
	_readCount = 0;
	_avgTime = 0;
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		_renderer.colorFill(_imageTexture, 0xff000000);
	}

	xn::AlternativeViewPointCapability viewPoint = _depth.GetAlternativeViewPointCap();
	viewPoint.SetViewPoint(_image);
	//viewPoint.ResetViewPoint();

	while (_worker) {
		timer.start();
		_context.WaitAndUpdateAll();
		_image.GetMetaData(_imageMD);
		_depth.GetMetaData(_depthMD);
		_user.GetUserPixels(0, _sceneMD);
		XnUserID userID[15];
		XnUInt16 users = 15;
		_user.GetUsers(userID, users);
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			_userID.clear();
			for (int i = 0; i < users; ++i) {
				_userID.push_back(userID[i]);
				if (_user.GetSkeletonCap().IsTracking(userID[i])) {
				}
			}
		}
		_readTime = timer.getTime();
		_readCount++;
		if (_readCount > 0) _avgTime = F(_avgTime * (_readCount - 1) + _readTime) / _readCount;

		D3DLOCKED_RECT lockedRect = {0};
		if SUCCEEDED(_imageSurface->LockRect(&lockedRect, NULL, 0)) {
			LPBYTE src = (LPBYTE)_imageMD.RGB24Data();
			LPBYTE dst = (LPBYTE)lockedRect.pBits;
			int pitchAdd = lockedRect.Pitch - SENSOR_WIDTH * 4;
			int i = 0;
			int j = 0;
			byte d, b,g,r;
			XnLabel l;
			for (int y = 0; y < SENSOR_HEIGHT; y++) {
				for (int x = 0; x < SENSOR_WIDTH; x++) {
					l = _sceneMD(x, y);
					d = 255 - (256 * (_depthMD(x, y) - DEPTH_RANGE_MIN) / DEPTH_RANGE_MAX);
					r = src[i++];
					g = src[i++];
					b = src[i++];
					switch (l) {
					case 0:
						dst[j++] = b;
						dst[j++] = g;
						dst[j++] = r;
						dst[j++] = d;
						break;
					case 1:
						dst[j++] = 0xff;
						dst[j++] = 0;
						dst[j++] = 0;
						dst[j++] = d;
						break;
					case 2:
						dst[j++] = 0;
						dst[j++] = 0;
						dst[j++] = 0xff;
						dst[j++] = d;
						break;
					case 3:
						dst[j++] = 0xff;
						dst[j++] = 0;
						dst[j++] = 0xff;
						dst[j++] = d;
						break;
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
			_renderer.copyTexture(_imageTexture, _texture);
		}
		_fpsCounter.count();
		Poco::Thread::sleep(1);
	}
}

void OpenNIScene::process() {
}

void OpenNIScene::draw1() {
}

void OpenNIScene::draw2() {
	if (_texture) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		DWORD col = 0xff0000ff;
		//_renderer.drawTexture(400, 0, 640, 480, NULL, 0, col, col, col, col);
		col = 0xffffffff;
		_renderer.drawTexture(400, 0, 640, 480, _texture, 0, col, col, col, col);
		//col = 0xccffffff;
		//_renderer.drawTexture(400, 0, 640, 480, _depthTexture, 0, col, col, col, col);
	}
	string s = Poco::format("%02lufps-%03.2hfms user-%?u", _fpsCounter.getFPS(), _avgTime, _userID.size());
	_renderer.drawFontTextureText(400, 0, 10, 10, 0xccff3333, s);
}

void XN_CALLBACK_TYPE newUser(xn::UserGenerator& generator, XnUserID id, void* cookie) {
	Poco::Logger& log = Poco::Logger::get("");
	log.information("new user");
}

void XN_CALLBACK_TYPE lostUser(xn::UserGenerator& generator, XnUserID id, void* cookie) {
	Poco::Logger& log = Poco::Logger::get("");
	log.information("lost user");
}

void XN_CALLBACK_TYPE detectedPose(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID id, void* cookie) {
	Poco::Logger& log = Poco::Logger::get("");
	log.information("detected pose");
}

void XN_CALLBACK_TYPE startCalibration(xn::SkeletonCapability& capability, XnUserID id, void* cookie) {
	Poco::Logger& log = Poco::Logger::get("");
	log.information("start calibration");
}

void XN_CALLBACK_TYPE endCalibration(xn::SkeletonCapability& capability, XnUserID id, XnBool success, void* cookie) {
	Poco::Logger& log = Poco::Logger::get("");
	log.information("end calibration");
}

#endif
