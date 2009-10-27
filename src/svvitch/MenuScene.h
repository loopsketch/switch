#pragma once

#include <Poco/ActiveMethod.h>

#include "Scene.h"
#include "ui/Button.h"
#include "ui/UserInterfaceManager.h"


class MenuScene: public Scene
{
private:
	Poco::FastMutex _lock;
	ui::UserInterfaceManager* _uim;
	ui::ButtonPtr _goOperation;
	ui::ButtonPtr _goEdit;
	ui::ButtonPtr _goSetup;


public:
	MenuScene(Renderer& renderer, ui::UserInterfaceManagerPtr uim);

	virtual ~MenuScene();

	virtual bool initialize();

	virtual void process();

	virtual void draw1();

	virtual void draw2();
};

typedef MenuScene* MenuScenePtr;