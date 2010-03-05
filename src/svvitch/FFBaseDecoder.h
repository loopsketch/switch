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
	FFBaseDecoder(Renderer& renderer, AVFormatContext* ic, const int streamNo);

	virtual ~FFBaseDecoder();

	/**
	 * �p�P�b�g���N���A���܂�
	 */
	void clearAllPackets();

	/**
	 * �p�P�b�g��
	 */
	const UINT bufferedPackets();

	/**
	 * �p�P�b�g������
	 */
	void pushPacket(AVPacket* packet);

	/**
	 * �p�P�b�g����o��
	 */
	AVPacketList* popPacket();


	/**
	 * ���σf�R�[�h����
	 */
	const float getAvgTime() const;
};
