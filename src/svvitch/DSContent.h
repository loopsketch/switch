#pragma once

#include "Content.h"
#include "Renderer.h"
#include "DSVideoRenderer.h"
#include "VideoTextureAllocator.h"
#include <Poco/ActiveMethod.h>
#include <Poco/Mutex.h>
#include <streams.h>


/**
 * DirectShow����Đ��R���e���g.
 */
class DSContent: public Content {
private:
	Poco::FastMutex _lock;
	IGraphBuilder* _gb;
	IBaseFilter* _vmr9;
	VideoTextureAllocatorPtr _allocator;
	DSVideoRendererPtr _vr;
	IMediaControl* _mc;
	IMediaSeeking* _ms;
	IMediaEvent* _me;
	bool _finished;

	ActiveMethod<void, void, DSContent> activePlay;
	void syncronizedPlay();

public:
	DSContent(Renderer& renderer, int splitType);

	virtual ~DSContent();


	void initialize();

	/** �t�@�C�����I�[�v�����܂� */
	bool open(const MediaItemPtr media, const int offset = 0);

	/**
	 * �Đ�
	 */
	void play();

	/**
	 * ��~
	 */
	void stop();

	/**
	 * �Đ������ǂ���
	 */
	const bool playing() const;

	const bool finished();

	/** �t�@�C�����N���[�Y���܂� */
	void close();

	void process(const DWORD& frame);

	void draw(const DWORD& frame);


	int getPinCount(IBaseFilter *pFilter, PIN_DIRECTION PinDir);

	/**
	 * �t�B���^�̎w�肵�������̐ڑ�����Ă��Ȃ��s����T���ĕԂ��܂��B
	 */
	bool getPin(IBaseFilter *pFilter, IPin** pPin, PIN_DIRECTION PinDir, int index = -1);

	bool getInPin(IBaseFilter *pFilter, IPin** pPin, int index = -1);

	bool getOutPin(IBaseFilter *pFilter, IPin** pPin, int index = -1);

	int dumpFilter(IGraphBuilder* gb);
};

typedef DSContent* DSContentPtr;
