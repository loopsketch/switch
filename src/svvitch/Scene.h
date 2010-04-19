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
	bool _visible;
	int _keycode;
	bool _shift;
	bool _ctrl;

	/** ステータス用のマップ */
	map<string, string> _status;

public:
	Scene(Renderer& renderer);

	virtual ~Scene();

	virtual bool initialize();

	/** 表示/非表示の設定 */
	void setVisible(const bool visible);

	virtual void notifyKey(const int keycode, const bool shift, const bool ctrl);

	/** ステータス情報の設定 */
	virtual void setStatus(const string& key, const string& value);

	/** ステータス情報の取得 */
	virtual const map<string, string>& getStatus();

	/** ステータス情報の取得 */
	virtual const string getStatus(const string& key);

	/** ステータス情報の削除 */
	virtual void removeStatus(const string& key);

	virtual void process();

	virtual void draw1();

	virtual void draw2();
};

typedef Scene* ScenePtr;