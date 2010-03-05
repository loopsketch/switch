#pragma once

#include "FFBaseDecoder.h"


class FFAudioDecoder: public FFBaseDecoder
{
friend class FFMovieContent;
private:
	static const int BUFFER_SIZE = AVCODEC_MAX_AUDIO_FRAME_SIZE * 3;

	LPDIRECTSOUNDBUFFER	_buffer;
	DWORD _bufferOffset;
	DWORD _bufferSize;

	bool _running;

	uint8_t* _data;
	int _dataOffset;
	int _len;


	FFAudioDecoder(Renderer& renderer, AVFormatContext* ic, const int streamNo);

	virtual ~FFAudioDecoder();


	void start();

	const UINT bufferedFrames();

	void decode();

	void writeData();

	bool playing();

	void play();

	void stop();
};
