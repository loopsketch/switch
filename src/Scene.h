#pragma once

#include "Renderer.h"
#include "Workspace.h"


class Scene
{
protected:
	Poco::Logger& _log;
	Renderer& _renderer;
	int _keycode;
	bool _shift;
	bool _ctrl;

public:
	Scene(Renderer& renderer);

	virtual ~Scene();

	virtual bool initialize();

	virtual void notifyKey(const int keycode, const bool shift, const bool ctrl);

	virtual void process();

	virtual void draw1();

	virtual void draw2();
};
