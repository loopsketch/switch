#pragma once

#include "Scene.h"
#include "ui/UserInterfaceManager.h"


class MenuScene: public Scene
{
private:
	Poco::FastMutex _lock;
	ui::UserInterfaceManager* _uim;

public:
	MenuScene(Renderer& renderer, ui::UserInterfaceManagerPtr uim);

	~MenuScene();

	virtual bool initialize();

	virtual void process();

	virtual void draw1();

	virtual void draw2();
};

typedef MenuScene* MenuScenePtr;