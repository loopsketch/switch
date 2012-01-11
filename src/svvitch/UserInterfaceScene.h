#pragma once

#include "Scene.h"
#include "ui/UserInterfaceManager.h"


/**
 * ユーザインターフェース管理シーンクラス.
 */
class UserInterfaceScene: public Scene
{
private:
	ui::UserInterfaceManager* _uim;

	DWORD _frame;

public:
	UserInterfaceScene(Renderer& renderer, ui::UserInterfaceManagerPtr uim);

	~UserInterfaceScene();

	virtual void process();

	virtual void draw1();

	virtual void draw2();
};

typedef UserInterfaceScene* UserInterfaceScenePtr;