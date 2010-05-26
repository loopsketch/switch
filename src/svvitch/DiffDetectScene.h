#pragma once

#include "Scene.h"
#include "streams.h"
//#include "DetectStatus.h"
#include <Poco/Mutex.h>
#include "FPSCounter.h"


class DiffDetectScene: public Scene, Poco::Runnable
{
private:
	Poco::FastMutex _lock;

	Poco::Thread _thread;
	Poco::Runnable* _worker;

	int _w;
	int _h;

	int _previewX;
	int _previewY;
	int _previewW;
	int _previewH;

	DWORD _frame;
	float _subtract;

	LPDIRECT3DTEXTURE9 _frame1;
	LPDIRECT3DTEXTURE9 _frame2;
	LPDIRECT3DTEXTURE9 _result;
	LPD3DXEFFECT _fx;
	LPBYTE _gray;

	DWORD _faceDetectTime;

	FPSCounter _fpsCounter;


	// èàóùÉXÉåÉbÉh
	void run();

	void drawDiff();

public:
	DiffDetectScene(Renderer& renderer);

	~DiffDetectScene();

	virtual bool initialize();

	virtual void process();

	virtual void draw1();

	virtual void draw2();
};

typedef DiffDetectScene* DiffDetectScenePtr;
