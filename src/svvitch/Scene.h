#pragma once

#include "Renderer.h"
#include "Workspace.h"
#include <string>
#include <map>

using std::string;
using std::map;


class Scene
{
protected:
	Poco::Logger& _log;
	Renderer& _renderer;
	int _keycode;
	bool _shift;
	bool _ctrl;

	/** ステータス用のマップ */
	map<string, string> _status;

public:
	Scene(Renderer& renderer);

	virtual ~Scene();

	virtual bool initialize();

	virtual void notifyKey(const int keycode, const bool shift, const bool ctrl);

	/** ステータス情報の取得 */
	virtual const map<string, string>& getStatus();

	virtual void process();

	virtual void draw1();

	virtual void draw2();
};

typedef Scene* ScenePtr;