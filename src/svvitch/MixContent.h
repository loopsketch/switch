#pragma once

#include <queue>
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>

#include "Content.h"
#include "Container.h"
#include "CaptureScene.h"
#include "MediaItem.h"
#include "PerformanceTimer.h"
#include "Renderer.h"
#include "Workspace.h"

using std::queue;
using std::string;
using std::wstring;


/**
 * 複合再生コンテントクラス.
 * 複数のコンテントを領域分割して同時に描画します
 */
class MixContent: public Content
{
private:
	Poco::FastMutex _lock;

	vector<ContentPtr> _contents;

	bool _playing;
	PerformanceTimer _playTimer;


public:
	MixContent(Renderer& renderer, int splitType);

	~MixContent();


	void initialize();

	/** ファイルをオープンします */
	bool open(const MediaItemPtr media, const int offset = 0);


	/** 再生 */
	void play();

	/** 停止 */
	void stop();

	bool useFastStop();

	/** 再生中かどうか */
	const bool playing() const;

	const bool finished();

	/** ファイルをクローズします */
	void close();

	void process(const DWORD& frame);

	void draw(const DWORD& frame);
};

typedef MixContent* MixContentPtr;