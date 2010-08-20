#pragma once

#include <string>
#include "Scene.h"
#include "flash.h"
#include "ControlSite.h"
#include <Poco/Mutex.h>

using std::string;
using std::wstring;


class FlashListener
{
public:
	virtual void FlashAnimEnded() {}
	virtual void FlashCommand(const std::string& theCommand, const std::string& theParam) {}
};


class FlashScene: public Scene
{
private:
	Poco::FastMutex _lock;

	int _x;
	int _y;
	int _w;
	int _h;

	HMODULE _module;
	ControlSite* _controlSite;
	IOleObject* _ole;
	IShockwaveFlash* _flash;
	IOleInPlaceObjectWindowless* _windowless;
	IViewObject* _view;

	LPDIRECT3DTEXTURE9 _buf;
	string _movie;

public:
	FlashScene(Renderer& renderer);

	virtual ~FlashScene();

	virtual bool initialize();

	virtual void process();

	virtual void draw1();

	virtual void draw2();

	long getReadyState();

	bool loadMovie(const string& file);

	bool isPlaying();

	int getCurrentFrame();
};

typedef FlashScene* FlashScenePtr;
