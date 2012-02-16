#pragma once

#include <winsock2.h>
#include "Scene.h"
#include <queue>
#include <Poco/Net/HTTPCookie.h>
#include <Poco/URI.h>

using std::queue;

/**
 * テロップ管理クラス
 */
class Telop {
private:
	Telop& copy(const Telop& t) {
		if (this == &t) return *this;
		x = t.x;
		y = t.y;
		w = t.w;
		h = t.h;
		texture = t.texture;
		return *this;
	}

public:
	int x;
	int y;
	int w;
	int h;
	LPDIRECT3DTEXTURE9 texture;

	Telop() {};
	//Telop(const Telop& t);
	virtual ~Telop() {};

	Telop& operator=(const Telop& t) {
		return copy(t);
    }
};


/**
 * ステージ上にテロップ表示を行います
 * RSS1.0/2.0をソースに利用できます
 */
class TelopScene: private Poco::Runnable, public Scene
{
private:
	Poco::FastMutex _lock;
	Poco::Thread _thread;
	Poco::Runnable* _worker;

	string _sourceURL;
	string _title;
	Poco::DateTime _date;
	int _validMinutes;
	int _speed;
	int _space;
	vector<string> _categories;
	bool _creating;
	bool _visible;

	queue<Telop> _prepared;
	vector<Telop> _telops;
	queue<Telop> _deletes;

	void run();

	map<string, vector<string>> readSource(const Poco::LocalDateTime& now);


public:
	TelopScene(Renderer& renderer);

	virtual ~TelopScene();

	virtual bool initialize();

	virtual void process();

	virtual void draw1();

	virtual void draw2();
};

typedef TelopScene* TelopScenePtr;
