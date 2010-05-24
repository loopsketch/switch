#pragma once

#include <Poco/Mutex.h>

#include "Content.h"
#include "PerformanceTimer.h"

using std::string;
using std::wstring;


class ImageContent: public Content
{
private: 
	Poco::FastMutex _lock;

	int _iw;
	int _ih;
	LPDIRECT3DTEXTURE9 _target;
	int _tw;
	int _th;
	float _dy;

	bool _finished;
	bool _playing;
	PerformanceTimer _playTimer;

public:
	ImageContent(Renderer& renderer);

	virtual ~ImageContent();


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

	virtual void process(const DWORD& frame);

	virtual void draw(const DWORD& frame);
};

typedef ImageContent* ImageContentPtr;