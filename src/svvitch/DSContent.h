#pragma once

#include "Content.h"
#include "Renderer.h"
#include "DSVideoRenderer.h"
#include "VideoTextureAllocator.h"
#include <Poco/ActiveMethod.h>
#include <Poco/Mutex.h>
#include <streams.h>


/**
 * DirectShow動画再生コンテント.
 */
class DSContent: public Content {
private:
	Poco::FastMutex _lock;
	IGraphBuilder* _gb;
	IBaseFilter* _vmr9;
	VideoTextureAllocatorPtr _allocator;
	DSVideoRendererPtr _vr;
	IMediaControl* _mc;
	IMediaSeeking* _ms;
	IMediaEvent* _me;
	bool _finished;

	ActiveMethod<void, void, DSContent> activePlay;
	void syncronizedPlay();

public:
	DSContent(Renderer& renderer, int splitType);

	virtual ~DSContent();


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

	/**
	 * 再生中かどうか
	 */
	const bool playing() const;

	const bool finished();

	/** ファイルをクローズします */
	void close();

	void process(const DWORD& frame);

	void draw(const DWORD& frame);


	int getPinCount(IBaseFilter *pFilter, PIN_DIRECTION PinDir);

	/**
	 * フィルタの指定した方向の接続されていないピンを探して返します。
	 */
	bool getPin(IBaseFilter *pFilter, IPin** pPin, PIN_DIRECTION PinDir, int index = -1);

	bool getInPin(IBaseFilter *pFilter, IPin** pPin, int index = -1);

	bool getOutPin(IBaseFilter *pFilter, IPin** pPin, int index = -1);

	int dumpFilter(IGraphBuilder* gb);
};

typedef DSContent* DSContentPtr;
