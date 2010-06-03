#pragma once

#include "Scene.h"
#include "streams.h"
//#include "DetectStatus.h"
#include <Poco/Mutex.h>
#include "FPSCounter.h"


class DiffDetectScene: public Scene
{
private:
	Poco::FastMutex _lock;

	int _w;
	int _h;

	int _previewX;
	int _previewY;
	int _previewW;
	int _previewH;

	DWORD _frame;
	int _samples;

	LPDIRECT3DTEXTURE9 _frame1;
	LPDIRECT3DTEXTURE9 _frame2;
	LPDIRECT3DTEXTURE9 _frame3;
	LPDIRECT3DTEXTURE9 _result1;
	LPDIRECT3DTEXTURE9 _result2;
	LPD3DXEFFECT _fx;


	void drawAverage(VERTEX* dst, DWORD& col);

	void drawEdge(VERTEX* dst, DWORD& col);

	void drawDiff(VERTEX* dst, DWORD& col);

public:
	DiffDetectScene(Renderer& renderer);

	~DiffDetectScene();

	virtual bool initialize();

	LPDIRECT3DTEXTURE9 getResult();

	virtual void process();

	virtual void draw1();

	virtual void draw2();
};

typedef DiffDetectScene* DiffDetectScenePtr;
