#pragma once

#include <Exdisp.h>
#include "ComContent.h"
#include "ControlSite.h"


/**
 * IEコンテントクラス.
 * 
 */
class IEContent: public ComContent {
private:
	HMODULE _module;
	IClassFactory* _classFactory;
	IViewObject* _view;

	string _url;

	virtual void createComComponents();

	virtual void releaseComComponents();

public:
	IEContent(Renderer& renderer, int splitType, float x = 0, float y = 0, float w = 0, float h = 0);

	virtual ~IEContent();

	void initialize();

	/** ファイルをオープンします */
	virtual bool open(const MediaItemPtr media, const int offset = 0);

	virtual void run();
};

typedef IEContent* IEContentPtr;