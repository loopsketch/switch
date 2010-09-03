#pragma once

//#include <windows.h>
//#include <queue>
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>

#include "Content.h"
#include "flash.h"
#include "ControlSite.h"


using std::queue;
using std::string;


class FlashContent: public Content {
private:
	Poco::FastMutex _lock;

	int _phase;
	HMODULE _module;
	ControlSite* _controlSite;
	IOleObject* _ole;
	IShockwaveFlash* _flash;
	IOleInPlaceObjectWindowless* _windowless;
	IViewObject* _view;

	LPDIRECT3DTEXTURE9 _texture;

	bool _playing;
	string _movie;
	PerformanceTimer _playTimer;

	void createFlashComponents();
	void releaseFlashComponents();

public:
	FlashContent(Renderer& renderer, float x = 0, float y = 0, float w = 0, float h = 0);

	virtual ~FlashContent();


	void initialize();

	/** ファイルをオープンします */
	bool open(const MediaItemPtr media, const int offset = 0);

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

	void draw(const DWORD& frame);
};

typedef FlashContent* FlashContentPtr;