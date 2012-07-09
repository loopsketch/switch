#ifdef USE_FFMPEG

#include <Poco/format.h>
#include <Poco/Logger.h>
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/UnicodeConverter.h>
#include "FFAudioDecoder.h"
#include "PerformanceTimer.h"


FFAudioDecoder::FFAudioDecoder(Renderer& renderer, AVFormatContext* ic, const int streamNo): FFBaseDecoder(renderer, ic, streamNo),
	_buffer(NULL), _bufferOffset(0), _bufferSize(0), _running(false), _data(NULL), _dataOffset(0), _len(0), _playCursor(0), _writeCursor(0)
{
}

FFAudioDecoder::~FFAudioDecoder() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	stop();
	SAFE_DELETE(_data);
	SAFE_RELEASE(_buffer);
}


bool FFAudioDecoder::isReady() {
	return true;
}

void FFAudioDecoder::start() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	AVCodecContext* avctx = _ic->streams[_streamNo]->codec;
	WORD sampleBit;
	string type;
	switch (avctx->sample_fmt) {
		case AV_SAMPLE_FMT_U8:
			type = "unsigned 8 bits";
			sampleBit = 8;
			break;
		case AV_SAMPLE_FMT_S16:
			type = "signed 16 bits";
			sampleBit = 16;
			break;
		case AV_SAMPLE_FMT_S32:
			type = "signed 32 bits";
			sampleBit = 32;
			break;
		case AV_SAMPLE_FMT_FLT:
			type = "float";
			sampleBit = 32;
			break;
		case AV_SAMPLE_FMT_DBL:
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
	// desc.dwFlags	= DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCDEFER | DSBCAPS_CTRLFREQUENCY;
	// desc.dwFlags	= DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCDEFER | DSBCAPS_CTRLPOSITIONNOTIFY;
	desc.dwFlags	= DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLVOLUME;
	desc.dwBufferBytes = _bufferSize;
	desc.lpwfxFormat = &wfwav;
	desc.guid3DAlgorithm = GUID_NULL;
	LPDIRECTSOUND sound = _renderer.getSoundDevice();
	HRESULT hr = sound->CreateSoundBuffer(&desc, &_buffer, NULL);
	if (FAILED(hr)) {
		_log.warning("failed not create sound buffer");

	} else {
		hr = _buffer->SetCurrentPosition(0);
		// DWORD frq;
		// _buffer->GetFrequency(&frq);
		// _buffer->SetFrequency(frq * 1.25);
		_data = new uint8_t[BUFFER_SIZE];
		ZeroMemory(_data, sizeof(uint8_t) * BUFFER_SIZE);
	}
}

const UINT FFAudioDecoder::bufferedFrames() {
	return bufferedPackets();
}

void FFAudioDecoder::decode() {
	if (!_data) return;
	//_log.information("audio decoder thread start");
	//DWORD threadAffinityMask = ::SetThreadAffinityMask(GetCurrentThread(), 1);
	if (_len > 0) return;
	if (_dataOffset > 0) return;
	AVPacketList* packetList = popPacket();
	if (!packetList) return;

	PerformanceTimer timer;
	timer.start();
	AVCodecContext* avctx = _ic->streams[packetList->pkt.stream_index]->codec;
	AVPacket* packet = &packetList->pkt;
	AVPacket tmp;
	av_init_packet(&tmp);
	tmp.data = packet->data;
	tmp.size = packet->size;
	while (tmp.data && tmp.size > 0) {
		int frameSize = sizeof(uint8_t) * BUFFER_SIZE;
		int bytes = avcodec_decode_audio3(avctx, (int16_t*)(&_data[_len]), &frameSize, &tmp);
		if (bytes >= 0) {
			tmp.data += bytes;
			tmp.size -= bytes;
			if (frameSize > 0) {
				_len += frameSize;
			}
			_readTime = timer.getTime();
			_readCount++;
			if (_readCount > 0) _avgTime = F(_avgTime * (_readCount - 1) + _readTime) / _readCount;

		} else {
			// throw error or something?
			_log.warning(Poco::format("failed avcodec_decode_audio3: %d %d", bytes, frameSize));
			_len = 0;
			break;
		}
	}
	if (packet->data) av_free_packet(packet);
	av_freep(&packetList);
}

void FFAudioDecoder::writeData() {
	if (!_buffer) return;

	HRESULT hr = _buffer->GetCurrentPosition(&_playCursor, &_writeCursor);
	if FAILED(hr) {
		_log.warning("failed get current position");
		return;
	}
	if (_len <= 0) return;

	if (_dataOffset > 0) {
		// 再生中で再生カーソルがバッファの半分以降になるまで書込み待ち
		// _log.information(Poco::format("buffer cursor: %lu %lu", _playCursor, (_bufferSize / 2)));
		if (!_running || _playCursor < _bufferSize / 2) return;

	} else {
		// 書込みカーソル以降の場合、再生カーソルのデータx2個分余裕ができるまで書込み待ち
		if (_writeCursor > _bufferOffset && _playCursor < _bufferOffset + _len * 2) return;
	}

	LPVOID lockedBuf = NULL;
	DWORD lockedLen = 0;
	if (_bufferOffset + _len <= _bufferSize) {
		// オフセット+データサイズがバッファ以下の場合はそのまますべて書込む
		hr = _buffer->Lock(_bufferOffset, _len, &lockedBuf, &lockedLen, NULL, 0, 0);
		if (SUCCEEDED(hr)) {
			CopyMemory(lockedBuf, &_data[_dataOffset], lockedLen);
			hr = _buffer->Unlock(lockedBuf, lockedLen, NULL, 0);
			if (FAILED(hr)) {
				_log.warning("failed unlocked sound buffer");
			} else {
				_bufferOffset = (_bufferOffset + _len) % _bufferSize;
				_len = 0;
				_dataOffset = 0;
			}
		} else {
			_log.warning(Poco::format("sound buffer not locked: %d", _len));
		}
	} else {
		// 残りバッファに書込みむ。残りは次回呼出し時に書込む
		//_log.information("round sound buffer");
		int lenRound = _bufferSize - _bufferOffset;
		hr = _buffer->Lock(_bufferOffset, lenRound, &lockedBuf, &lockedLen, NULL, 0, 0);
		if (SUCCEEDED(hr)) {
			CopyMemory(lockedBuf, _data, lockedLen);
			hr = _buffer->Unlock(lockedBuf, lockedLen, NULL, 0);
			if (FAILED(hr)) {
				_log.warning("failed unlocked sound buffer");
			} else {
				_bufferOffset = 0;
				_len -= lenRound;
				_dataOffset = lenRound;
			}
		} else {
			_log.warning(Poco::format("SoundBuffer not locked: %d", lenRound));
		}
	}
}

void FFAudioDecoder::finishedPacket() {
	if (!_buffer) return;

	int size = _bufferSize - _bufferOffset;
	LPVOID lockedBuf = NULL;
	DWORD lockedLen = 0;
	HRESULT hr = _buffer->Lock(_bufferOffset, size, &lockedBuf, &lockedLen, NULL, 0, 0);
	if (SUCCEEDED(hr)) {
		ZeroMemory(lockedBuf, lockedLen);
		hr = _buffer->Unlock(lockedBuf, lockedLen, NULL, 0);
	}
}

bool FFAudioDecoder::playing() {
	return _running;
}

void FFAudioDecoder::play() {
	if (_buffer && !_running) {
		HRESULT hr = _buffer->Play(0, 0, DSBPLAY_LOOPING);
		if (SUCCEEDED(hr)) {
			_running = true;
		} else {
			_log.warning("failed play sound buffer");
		}
	}
}

void FFAudioDecoder::stop() {
	if (_buffer && _running) {
		HRESULT hr = _buffer->Stop();
		if (SUCCEEDED(hr)) {
			_running = false;
		}
	}
}

#endif
