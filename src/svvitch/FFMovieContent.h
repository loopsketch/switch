#pragma once

#include "FFmpeg.h"

#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>
#include "FFAudioDecoder.h"
#include "FFVideoDecoder.h"

#include "Content.h"
#include "FPSCounter.h"
#include "MediaItem.h"
#include "PerformanceTimer.h"
#include "Renderer.h"

using std::string;
using std::wstring;


class FFMovieContent: public Content, Poco::Runnable
{
private:
	Poco::FastMutex _lock;
	Poco::FastMutex _openLock;
	Poco::FastMutex _frameLock;

	Poco::Thread _thread;
	Poco::Runnable* _worker;

	AVFormatContext* _ic;
	int _fps;
	int _video;
	int _audio;
	FFAudioDecoder* _audioDecoder;
	FFVideoDecoder* _videoDecoder;
	VideoFrame* _vf;
	VideoFrame* _prepareVF;

	bool _starting;
	int _frameOddEven;
	bool _finished;
	bool _seeking;
	PerformanceTimer _playTimer;
	FPSCounter _fpsCounter;
	float _avgTime;

public:
	FFMovieContent(Renderer& renderer, int splitType);

	virtual ~FFMovieContent();


	void initialize();

	/** ファイルをオープンします */
	bool open(const MediaItemPtr media, const int offset = 0);

	void run();

	/**
	 * 再生
	 */
	void play();

	/**
	 * 停止
	 */
	void stop();

	/**
	 * Seek to the keyframe at timestamp.
	 * 'timestamp' in 'stream_index'.
	 * @param stream_index If stream_index is (-1), a default
	 * stream is selected, and timestamp is automatically converted
	 * from AV_TIME_BASE units to the stream specific time_base.
	 * @param timestamp Timestamp in AVStream.time_base units
	 *        or, if no stream is specified, in AV_TIME_BASE units.
	 * @param flags flags which select direction and seeking mode
	 * @return >= 0 on success
	 */
	const bool seek(const int64_t timestamp);

	const bool finished();

	/** ファイルをクローズします */
	void close();

	virtual void process(const DWORD& frame);

	virtual void draw(const DWORD& frame);

	/**
	 * 再生フレームレート
	 */
	const Uint32 getFPS();

	/**
	 * 平均デコード時間
	 */
	const float getAvgTime() const;

	/**
	 * 現在の再生時間
	 */
	const DWORD currentTime();

	/**
	 * 残り時間
	 */
	const DWORD timeLeft();
};

typedef FFMovieContent* FFMovieContentPtr;