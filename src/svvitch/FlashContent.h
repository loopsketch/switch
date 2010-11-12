#pragma once

#include "ComContent.h"
#include "flash.h"


using std::string;


class FlashContent: public ComContent {
private:
	HMODULE _module;
	IClassFactory* _classFactory;

	string _movie;
	string _params;
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