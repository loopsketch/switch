#pragma once

#include "Content.h"
#include "Renderer.h"
#include "DSVideoRenderer.h"
#include <Poco/Mutex.h>
#include <streams.h>


class DSContent: public Content {
private:
	Poco::FastMutex _lock;
	IGraphBuilder* _gb;
	DSVideoRendererPtr _vr;
	IMediaControl* _mc;
	IMediaSeeking* _ms;
	IMediaEvent* _me;

public:
	DSContent(Renderer& renderer);

	~DSContent();


	void initialize();

	/** ファイルをオープンします */
	bool open(const MediaItemPtr media, const int offset = 0);

	const MediaItemPtr opened() const;

	/**
	 * 再生
	 */
	void play();

	/**
	 * 停止
	 */
	void stop();

	/**
	 * 再生中かどうか
	 */
	const bool playing() const;

	const bool finished();

	/** ファイルをクローズします */
	void close();

	void process(const DWORD& frame);

	void draw(const DWORD& frame);


	bool getPin(IBaseFilter *pFilter, IPin** pPin, PIN_DIRECTION PinDir);
	bool getInPin(IBaseFilter *pFilter, IPin** pPin);
	bool getOutPin(IBaseFilter *pFilter, IPin** pPin);
	int dumpFilter(IGraphBuilder* gb);
};

typedef DSContent* DSContentPtr;
