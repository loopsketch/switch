#pragma once

#include <Poco/Mutex.h>

#include "Content.h"
#include "PerformanceTimer.h"

using std::string;
using std::wstring;


/**
 * �Î~��R���e���g�N���X.
 * �Î~��t�@�C����`�悷��R���e���g�ł�
 */
class ImageContent: public Content
{
private: 
	Poco::FastMutex _lock;

	int _iw;
	int _ih;
	LPDIRECT3DTEXTURE9 _target;
	int _tw;
	int _th;
	float _dx;
	float _dy;
	float _rx;

	bool _finished;
	bool _playing;
	PerformanceTimer _playTimer;

public:
	ImageContent(Renderer& renderer, int splitType, float x = 0, float y = 0, float w = 0, float h = 0);

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