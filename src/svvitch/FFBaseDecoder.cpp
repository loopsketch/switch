#ifdef USE_FFMPEG

#include "FFBaseDecoder.h"
#include <Poco/format.h>

FFBaseDecoder::FFBaseDecoder(Renderer& renderer, AVFormatContext* ic, const int streamNo):
	_log(Poco::Logger::get("")), _renderer(renderer), _ic(ic), _streamNo(streamNo), _readTime(0), _readCount(0), _avgTime(0) {
}

FFBaseDecoder::~FFBaseDecoder() {
	clearAllPackets();
}

void FFBaseDecoder::clearAllPackets() {
	//_log.information("clear packets");
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	int count = 0;
	while (_packets.size() > 0) {
		AVPacketList* packetList = _packets.front();
		_packets.pop();
		av_free_packet(&packetList->pkt);
		av_freep(&packetList);
	}
	//_log.information(Poco::format("clear packets: %d", count));
}

const UINT FFBaseDecoder::bufferedPackets() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	return _packets.size();
}

void FFBaseDecoder::pushPacket(AVPacket* packet) {
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

AVPacketList* FFBaseDecoder::popPacket() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	AVPacketList* packetList = NULL;
	if (_packets.size() > 0) {
		packetList = _packets.front();
		_packets.pop();
	}
	return packetList;
}

const float FFBaseDecoder::getAvgTime() const {
	return _avgTime;
}

#endif
