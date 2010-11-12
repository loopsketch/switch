#pragma once

#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>

#include "ComContent.h"
#include "flash.h"
#include "ControlSite.h"


using std::string;


class FlashContent: public ComContent, Poco::Runnable {
private:
	int _phase;
	HMODULE _module;
	IClassFactory* _classFactory;
	ControlSite* _controlSite;
	IOleObject* _ole;
	Poco::Thread _thread;
	Poco::Runnable* _worker;

	LPDIRECT3DTEXTURE9 _texture;
	LPDIRECT3DSURFACE9 _surface;
	HDC _hdc;

	string _movie;
	string _params;
	PerformanceTimer _playTimer;
	DWORD _background;
	int _zoom;

	DWORD _readTime;
	int _readCount;
	float _avgTime;

	virtual void createComComponents();

	virtual void releaseComComponents();

public:
	FlashContent(Renderer& renderer, int splitType, float x = 0, float y = 0, float w = 0, float h = 0);

	virtual ~FlashContent();

	void initialize();

	/** ファイルをオープンします */
	bool open(const MediaItemPtr media, const int offset = 0);

	void update();

	/**
	 * 再生
	 */
	void play();

	/**
	 * 停止
	 */
	void stop();

	bool useFastStop();

	/**
	 * 再生中かどうか
	 */
	const bool playing() const;

	const bool finished();

	/** ファイルをクローズします */
	void close();

	void process(const DWORD& frame);

	void run();

	void draw(const DWORD& frame);
};

typedef FlashContent* FlashContentPtr;