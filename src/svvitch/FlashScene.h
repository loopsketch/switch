#pragma once

#include "Scene.h"
#include "ControlSite.h"


class FlashScene: public Scene
{
private:
	ControlSite* _controlSite;
	IOleObject* _ole;
	ShockwaveFlashObjects::IShockwaveFlash* _flash;
	IOleInPlaceObjectWindowless* _windowless;
	IViewObject* _view;

public:
	FlashScene(Renderer& renderer);

	virtual ~FlashScene();

	virtual bool initialize();

	virtual void process();

	virtual void draw1();

	virtual void draw2();
};

typedef FlashScene* FlashScenePtr;
