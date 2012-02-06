#pragma once

#include "Scene.h"
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>
#include <XnCppWrapper.h>
#include "FPSCounter.h"

using std::vector;

#pragma comment(lib, "openNI.lib")

#define SENSOR_WIDTH	640
#define SENSOR_HEIGHT	480
#define DEPTH_RANGE_MIN	500
#define DEPTH_RANGE_MAX	5500


/**
 * 差分検出シーンクラス.
 * 差分検出の機能を提供します
 */
class OpenNIScene: public Scene, Poco::Runnable
{
private:
	Poco::FastMutex _lock;
	Poco::Thread _thread;
	Poco::Runnable* _worker;

	xn::Context _context;
	xn::ImageGenerator _image;
	xn::ImageMetaData _imageMD;
	xn::DepthGenerator _depth;
	xn::DepthMetaData _depthMD;
	xn::UserGenerator _user;
	xn::SceneMetaData _sceneMD;
	XnChar _pose[20];
	vector<XnUserID> _userID;

	LPDIRECT3DTEXTURE9 _imageTexture;
	LPDIRECT3DSURFACE9 _imageSurface;
	LPDIRECT3DTEXTURE9 _texture;

	FPSCounter _fpsCounter;
	DWORD _readTime;
	int _readCount;
	float _avgTime;

public:
	OpenNIScene(Renderer& renderer);

	virtual ~OpenNIScene();

	virtual bool initialize();

	void run();

	virtual void process();

	virtual void draw1();

	virtual void draw2();
};

void XN_CALLBACK_TYPE newUser(xn::UserGenerator& generator, XnUserID id, void* cookie);
void XN_CALLBACK_TYPE lostUser(xn::UserGenerator& generator, XnUserID id, void* cookie);
void XN_CALLBACK_TYPE detectedPose(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID id, void* cookie);
void XN_CALLBACK_TYPE startCalibration(xn::SkeletonCapability& capability, XnUserID id, void* cookie);
void XN_CALLBACK_TYPE endCalibration(xn::SkeletonCapability& capability, XnUserID id, XnBool success, void* cookie);

typedef OpenNIScene* OpenNIScenePtr;