#pragma once

#include "Content.h"
#include "Renderer.h"
#include <Poco/ActiveMethod.h>
#include <Poco/Mutex.h>
#include <mfapi.h>
#include <mfidl.h>

#pragma comment(lib, "mf.lib")


/**
 * MediaFoundation動画再生コンテントクラス.
 */
class MFContent: public Content {
private:
	Poco::FastMutex _lock;
	bool _finished;
	IMFMediaSession* _session;

public:
	MFContent(Renderer& renderer, int splitType);

	virtual ~MFContent();


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
};

typedef MFContent* MFContentPtr;
