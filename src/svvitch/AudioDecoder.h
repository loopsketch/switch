#pragma once

#include <Poco/Logger.h>
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>
#include <Poco/format.h>
#include <Poco/UnicodeConverter.h>
#include <queue>
#include <string>
#include <mmsystem.h>

#include "BaseDecoder.h"
#include "Renderer.h"
#include "PerformanceTimer.h"


class AudioDecoder: public BaseDecoder, Poco::Runnable
{
friend class FFMovieContent;
private:
	Renderer& _renderer;
	AVFormatContext* _ic;
	int _audio;

	LPDIRECTSOUNDBUFFER	_buffer;
	DWORD _bufferOffset;
	DWORD _bufferSize;
	bool _bufferReady;

	bool _running;

	DWORD _readTime;
	int _readCount;


	AudioDecoder(Renderer& renderer, AVFormatContext* ic, const int audio): BaseDecoder(),
		_renderer(renderer), _ic(ic), _audio(audio), _buffer(NULL), _bufferOffset(0), _bufferSize(0), _bufferReady(false), _running(false)
	{
	}

	virtual ~AudioDecoder() {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		_worker = NULL;
		_thread.join();

		SAFE_RELEASE(_buffer);
	}


	void start() {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		AVCodecContext* avctx = _ic->streams[_audio]->codec;
		WORD sampleBit;
		string type;
		switch (avctx->sample_fmt) {
			case SAMPLE_FMT_U8:
				type = "unsigned 8 bits";
				sampleBit = 8;
				break;
			case SAMPLE_FMT_S16:
				type = "signed 16 bits";
				sampleBit = 16;
				break;
			case SAMPLE_FMT_S32:
				type = "signed 32 bits";
				sampleBit = 32;
				break;
			case SAMPLE_FMT_FLT:
				type = "float";
				sampleBit = 32;
				break;
			case SAMPLE_FMT_DBL:
				type = "double";
				sampleBit = 32;
				break;
			default:
				type = Poco::format("unknown format(%d)", (int)avctx->sample_fmt);
		}
		_log.information(Poco::format("audio stream: format(%s) channels: %d sample: %hubit %dHz bitrate: %d", type, avctx->channels, sampleBit, avctx->sample_rate, avctx->bit_rate));

		//WAVEフォーマット設定
		WAVEFORMATEX wfwav;
		ZeroMemory(&wfwav, sizeof(wfwav));
		wfwav.wFormatTag = WAVE_FORMAT_PCM;
		wfwav.nChannels = avctx->channels;
		wfwav.nSamplesPerSec = avctx->sample_rate;
		wfwav.wBitsPerSample = sampleBit;
		wfwav.nBlockAlign = wfwav.nChannels * wfwav.wBitsPerSample / 8;
		wfwav.nAvgBytesPerSec = wfwav.nSamplesPerSec * wfwav.nBlockAlign;
		wfwav.cbSize = 0;

		_bufferSize = wfwav.nAvgBytesPerSec * 20;
		_bufferOffset = 0;
		_log.information(Poco::format("sound buffer size: %lu", _bufferSize));

		//プライマリバッファの作成
		DSBUFFERDESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.dwSize		= sizeof(desc);
		desc.dwFlags	= DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCDEFER | DSBCAPS_CTRLFREQUENCY;
		desc.dwBufferBytes = _bufferSize;
		desc.lpwfxFormat = &wfwav;
		desc.guid3DAlgorithm = GUID_NULL;
		LPDIRECTSOUND sound = _renderer.getSoundDevice();
		HRESULT hr = sound->CreateSoundBuffer(&desc, &_buffer, NULL);
		if (FAILED(hr)) {
			_log.warning("failed not create sound buffer");

		} else {
//			DWORD frq;
//			_buffer->GetFrequency(&frq);
//			_buffer->SetFrequency(frq * 1.25);
			_worker = this;
			_thread.start(*_worker);
		}
	}

	const UINT bufferedFrames() {
		return bufferedPackets();
	}

	void run() {
		_log.information("audio decoder thread start");
		PerformanceTimer timer;

		const int BUFFER_SIZE = AVCODEC_MAX_AUDIO_FRAME_SIZE * 3;
		uint8_t* data = new uint8_t[BUFFER_SIZE];
		ZeroMemory(data, sizeof(uint8_t) * BUFFER_SIZE);
		_readCount = 0;
		_avgTime = 0;

		AVPacketList* packetList = NULL;
		while (_worker) {
			packetList = popPacket();
			if (!packetList) {
				Poco::Thread::sleep(17);
				continue;
			}
			AVCodecContext* avctx = _ic->streams[packetList->pkt.stream_index]->codec;
			timer.start();

			AVPacket* packet = &packetList->pkt;
			AVPacket tmp;
			av_init_packet(&tmp);
			tmp.data = packet->data;
			tmp.size = packet->size;
			int len = 0;
			while (_worker && tmp.data && tmp.size > 0) {
				int frameSize = sizeof(uint8_t) * BUFFER_SIZE;
				int bytes = avcodec_decode_audio3(avctx, (int16_t*)(&data[len]), &frameSize, &tmp);
				if (bytes >= 0) {
					tmp.data += bytes;
					tmp.size -= bytes;
					if (frameSize > 0) {
						len += frameSize;
					}
					_readTime = timer.getTime();
					_readCount++;
					_avgTime = F(_avgTime * (_readCount - 1) + _readTime) / _readCount;

				} else {
					// throw error or something?
					_log.warning(Poco::format("failed avcodec_decode_audio3: %d %d", bytes, frameSize));
					len = 0;
					break;
				}
			}
			if (packet->data) av_free_packet(packet);
			av_freep(&packetList);

			if (len > 0) {
//				UINT adrs = (UINT)write1;
//				_log.information(Poco::format("audio frame: %X %d %lu %lu", adrs, len, len1, len2));
				LPVOID lockedBuf = NULL;
				DWORD lockedLen = 0;
				if (_bufferOffset + len <= _bufferSize) {
					HRESULT hr = _buffer->Lock(_bufferOffset, len, &lockedBuf, &lockedLen, NULL, 0, 0);
					if (SUCCEEDED(hr)) {
						CopyMemory(lockedBuf, data, lockedLen);
						hr = _buffer->Unlock(lockedBuf, lockedLen, NULL, 0);
						if (FAILED(hr)) {
							_log.warning("failed unlocked sound buffer");
						} else {
							_bufferOffset = (_bufferOffset + len) % _bufferSize;
							_bufferReady = true;
						}
					} else {
						_log.warning(Poco::format("SoundBuffer not locked: %d", len));
					}
				} else {
					int lenRound = _bufferSize - _bufferOffset;
					HRESULT hr = _buffer->Lock(_bufferOffset, lenRound, &lockedBuf, &lockedLen, NULL, 0, 0);
					if (SUCCEEDED(hr)) {
						CopyMemory(lockedBuf, data, lockedLen);
						hr = _buffer->Unlock(lockedBuf, lockedLen, NULL, 0);
						if (FAILED(hr)) {
							_log.warning("failed unlocked sound buffer");
						} else {
							_bufferOffset = 0;
							len -= lenRound;
							_bufferReady = true;
						}
					} else {
						_log.warning(Poco::format("SoundBuffer not locked: %d", lenRound));
					}
					while (_worker && !_running) {
						Poco::Thread::sleep(17);
					}
					hr = _buffer->Lock(_bufferOffset, len, &lockedBuf, &lockedLen, NULL, 0, 0);
					if (SUCCEEDED(hr)) {
						CopyMemory(lockedBuf, &data[lenRound], lockedLen);
						hr = _buffer->Unlock(lockedBuf, lockedLen, NULL, 0);
						if (FAILED(hr)) {
							_log.warning("failed unlocked sound buffer");
						} else {
							_bufferOffset = (_bufferOffset + len) % _bufferSize;
							_bufferReady = true;
						}
					} else {
						_log.warning(Poco::format("SoundBuffer not locked: %d", len));
					}
				}
			}

//			timeBeginPeriod(1);
			Poco::Thread::sleep(2);
//			timeEndPeriod(1);
		}

		delete data;
		_worker = NULL;
		_log.information("audio decoder thread end");
	}

	bool playing() {
		return _running;
	}

	void play() {
		if (_buffer && !_running) {
			if (_bufferReady) {
				HRESULT hr = _buffer->Play(0, 0, DSBPLAY_LOOPING);
				if (SUCCEEDED(hr)) {
					_running = true;
				} else {
					_log.warning("failed play sound buffer");
				}
			} else {
				_log.warning("buffer not ready!");
			}
		}
	}

	void stop() {
		if (_buffer && _running) {
			_buffer->Stop();
			_running = false;
		}
	}
};
