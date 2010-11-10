#pragma once

#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>

#include "ComContent.h"
#include "flash.h"
#include "ControlSite.h"


using std::queue;
using std::string;


class FlashContent: public ComContent, Poco::Runnable {
private:
	int _phase;
	HMODULE _module;
	IClassFactory* _classFactory;
	ControlSite* _controlSite;
	IOleObject* _ole;
	IShockwaveFlash* _flash;
	IOleInPlaceObjectWindowless* _windowless;
	IViewObject* _view;
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

	void createFlashComponents();
	void releaseFlashComponents();

public:
	FlashContent(Renderer& renderer, int splitType, float x = 0, float y = 0, float w = 0, float h = 0);

	virtual ~FlashContent();


	void initialize();

	/** �t�@�C�����I�[�v�����܂� */
	bool open(const MediaItemPtr media, const int offset = 0);

	void update();

	/**
	 * �Đ�
	 */
	void play();

	/**
	 * ��~
	 */
	void stop();

	bool useFastStop();

	/**
	 * �Đ������ǂ���
	 */
	const bool playing() const;

	const bool finished();

	/** �t�@�C�����N���[�Y���܂� */
	void close();

	void process(const DWORD& frame);

	void run();

	void draw(const DWORD& frame);
};

typedef FlashContent* FlashContentPtr;