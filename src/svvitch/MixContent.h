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

	/** �t�@�C�����I�[�v�����܂� */
	bool open(const MediaItemPtr media, const int offset = 0);


	/** �Đ� */
	void play();

	/** ��~ */
	void stop();

	bool useFastStop();

	/** �Đ������ǂ��� */
	const bool playing() const;

	const bool finished();

	/** �t�@�C�����N���[�Y���܂� */
	void close();

	void process(const DWORD& frame);

	void draw(const DWORD& frame);
};

typedef MixContent* MixContentPtr;