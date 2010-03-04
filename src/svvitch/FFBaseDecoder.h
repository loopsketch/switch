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
	FFBaseDecoder(Renderer& renderer, AVFormatContext* ic, const int streamNo): _log(Poco::Logger::get("")), _renderer(renderer), _ic(ic), _streamNo(streamNo), _readTime(0), _readCount(0), _avgTime(0) {
	}

	virtual ~FFBaseDecoder() {
		clearAllPackets();
	}

	/**
	 * パケットをクリアします
	 */
	void clearAllPackets() {
		_log.information("clear packets");
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		int count = 0;
		while (_packets.size() > 0) {
			AVPacketList* packetList = _packets.front();
			_packets.pop();
			av_free_packet(&packetList->pkt);
			av_freep(&packetList);
		}
		_log.information(Poco::format("clear packets: %d", count));
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
