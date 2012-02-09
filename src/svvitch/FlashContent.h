#pragma once

#include "ComContent.h"
#include "flash.h"
#include <string>
#include <list>

using std::string;
using std::list;


/**
 * Flashコンテントクラス.
 * Flashを描画するためのコンテント
 */
class FlashContent: public ComContent {
private:
	HMODULE _module;
	IClassFactory* _classFactory;
	IShockwaveFlash* _flash;
	IViewObject* _view;

	string _movie;
	string _params;
	string _quality;
	string _scale;
	int _zoom;

	virtual void createComComponents();

	virtual void releaseComComponents();

public:
	FlashContent(Renderer& renderer, int splitType, float x = 0, float y = 0, float w = 0, float h = 0);

	virtual ~FlashContent();

	void initialize();

	/** ファイルをオープンします */
	virtual bool open(const MediaItemPtr media, const int offset = 0);

	virtual void run();
};

typedef FlashContent* FlashContentPtr;