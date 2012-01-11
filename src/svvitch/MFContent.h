#pragma once

#include "Content.h"
#include "Renderer.h"
#include <Poco/ActiveMethod.h>
#include <Poco/Mutex.h>
#include <mfapi.h>
#include <mfidl.h>

#pragma comment(lib, "mf.lib")


/**
 * MediaFoundation����Đ��R���e���g�N���X.
 */
class MFContent: public Content {
private:
	Poco::FastMutex _lock;
	bool _finished;
	IMFMediaSession* _session;

public:
	MFContent(Renderer& renderer, int splitType);

	virtual ~MFContent();


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
};

typedef MFContent* MFContentPtr;
