#pragma once

#include <Poco/Logger.h>
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>
#include <queue>

#include "FFBaseDecoder.h"
#include "VideoFrame.h"

using std::queue;


class FFVideoDecoder: public FFBaseDecoder, Poco::Runnable
{
friend class FFMovieContent;
private:
	Poco::FastMutex _startLock;

	Poco::Thread _thread;
	Poco::Runnable* _worker;

	SwsContext* _swsCtx;
	AVFrame* _outFrame;
	uint8_t* _buffer;

	AVFrame* _diFrame;
	uint8_t* _diBuffer;

	queue<VideoFrame*> _frames;
	queue<VideoFrame*> _usedFrames;

	LPD3DXEFFECT _fx;

	int _dw;
	int _dh;

	FFVideoDecoder(Renderer& renderer, AVFormatContext* ic, const int streamNo);

	virtual ~FFVideoDecoder();


	/**
	 * フレームを全てクリアします
	 */
	void clearAllFrames();

	void start();

	const float getDisplayAspectRatio() const;

	const UINT bufferedFrames();

	void run();

	VideoFrame* parseAVFrame(AVCodecContext* avctx, AVFrame* frame);

	void pushUsedFrame(VideoFrame* vf);

	VideoFrame* popFrame();

	VideoFrame* frontFrame();

	VideoFrame* viewFrame();

	VideoFrame* popUsedFrame();
};
