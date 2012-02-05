#pragma once

#include "Scene.h"
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>
#include <XnCppWrapper.h>
#include "FPSCounter.h"

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

typedef OpenNIScene* OpenNIScenePtr;