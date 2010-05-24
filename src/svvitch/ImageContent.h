#pragma once

#include <Poco/Mutex.h>

#include "Content.h"
#include "PerformanceTimer.h"

using std::string;
using std::wstring;


class ImageContent: public Content
{
private: 
	Poco::FastMutex _lock;

	int _iw;
	int _ih;
	LPDIRECT3DTEXTURE9 _target;
	int _tw;
	int _th;
	float _dy;

	bool _finished;
	bool _playing;
	PerformanceTimer _playTimer;

public:
	ImageContent(Renderer& renderer);

	virtual ~ImageContent();


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

	virtual void process(const DWORD& frame);

	virtual void draw(const DWORD& frame);
};

typedef ImageContent* ImageContentPtr;