#pragma once

#include <queue>
#include <Poco/Logger.h>
#include <Poco/Mutex.h>

#include "FFmpeg.h"

#include "Renderer.h"

using std::queue;


class FFBaseDecoder
{
private:
	queue<AVPacketList*> _packets;

protected:
	Poco::FastMutex _lock;
	Poco::Logger& _log;

	Renderer& _renderer;
	AVFormatContext* _ic;
	int _streamNo;

	DWORD _readTime;
	int _readCount;
	float _avgTime;

public:
	FFBaseDecoder(Renderer& renderer, AVFormatContext* ic, const int streamNo);

	virtual ~FFBaseDecoder();

	virtual bool isReady() = 0;

	/**
	 * パケットをクリアします
	 */
	void clearAllPackets();

	/**
	 * パケット数
	 */
	const UINT bufferedPackets();

	/**
	 * パケットを入れる
	 */
	void pushPacket(AVPacket* packet);

	/**
	 * パケットを取出す
	 */
	AVPacketList* popPacket();


	/**
	 * 平均デコード時間
	 */
	const float getAvgTime() const;
};
