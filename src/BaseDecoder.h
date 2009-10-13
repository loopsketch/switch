#pragma once

#include <queue>
#include <Poco/Logger.h>
#include <Poco/Mutex.h>

extern "C" {
#define inline _inline
#include <libavutil/avstring.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

using std::queue;


class BaseDecoder
{
private:
	Poco::FastMutex _lock;
	queue<AVPacketList*> _packets;

protected:
	Poco::Logger& _log;

	DWORD _readTime;
	int _readCount;
	float _avgTime;

public:
	BaseDecoder(): _log(Poco::Logger::get("")), _avgTime(0) {
	}

	virtual ~BaseDecoder() {
		clearAllPackets();
	}


	/**
	 * パケットをクリアします
	 */
	void clearAllPackets() {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		while (_packets.size() > 0) {
			AVPacketList* packetList = _packets.front();
			_packets.pop();
			av_free_packet(&packetList->pkt);
			av_freep(&packetList);
		}
	}

	/**
	 * パケット数
	 */
	const UINT bufferedPackets() {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		return _packets.size();
	}

	/**
	 * パケットを入れる
	 */
	void pushPacket(AVPacket* packet) {
		if (av_dup_packet(packet) < 0) {
			_log.warning("failed av_dup_packet()");
		} else {
			AVPacketList* packetList = (AVPacketList*)av_malloc(sizeof(AVPacketList));
			if (!packetList) return;
			packetList->pkt = *packet;
			packetList->next = NULL;
			{
				Poco::ScopedLock<Poco::FastMutex> lock(_lock);
				_packets.push(packetList);
			}
		}
	}

	/**
	 * パケットを取出す
	 */
	AVPacketList* popPacket() {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		AVPacketList* packetList = NULL;
		if (_packets.size() > 0) {
			packetList = _packets.front();
			_packets.pop();
		}
		return packetList;
	}


	/**
	 * 平均デコード時間
	 */
	const float getAvgTime() const {
		return _avgTime;
	}
};
