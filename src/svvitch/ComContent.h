#pragma once

#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>

#include "Content.h"
#include "ControlSite.h"


using std::queue;

/**
 * �̈�N���X.
 * �̈�l�̃z���_
 */
class Rect
{
private:
	/** �̈�̃R�s�[���� */
	Rect& copy(const Rect& rect) {
		if (this == &rect) return *this;
		x = rect.x;
		y = rect.y;
		w = rect.w;
		h = rect.h;
		return *this;
	}

public:
	int x;
	int y;
	int w;
	int h;

	Rect(int x_, int y_, int w_, int h_): x(x_), y(y_), w(w_), h(h_) {
	}

	Rect(const Rect& rect) {
		copy(rect);
	}

	Rect& operator=(const Rect& rect) {
		return copy(rect);
    }
};


/**
 * COM(ActiveX)�̃R���e���g�N���X.
 * COM��`�悷��N���X�ł�
 */
class ComContent: public Content, Poco::Runnable {
private:
protected:
	Poco::FastMutex _lock;
	queue<Rect> _invalidateRects;

	IOleObject* _ole;
	ControlSite* _controlSite;
	Poco::Thread _thread;
	Poco::Runnable* _worker;

	LPDIRECT3DTEXTURE9 _texture;
	LPDIRECT3DSURFACE9 _surface;

	int _phase;
	DWORD _background;
	PerformanceTimer _playTimer;
	DWORD _readTime;
	int _readCount;
	float _avgTime;

	/** �R���X�g���N�^ */
	ComContent(Renderer& renderer, int splitType, float x = 0, float y = 0, float w = 0, float h = 0);

	/** �f�X�g���N�^ */
	virtual ~ComContent();

	/** COM���� */
	virtual void createComComponents() = 0;

	/** COM�J�� */
	virtual void releaseComComponents() = 0;

	/** invalidate�̈悪���邩 */
	bool hasInvalidateRect();

	/** invalidate�̈����o���܂� */
	Rect popInvalidateRect();


public:
	/** invalidate�̈��ʒm���܂� */
	void invalidateRect(int x, int y, int w, int h);

	/** �t�@�C�����I�[�v�����܂� */
	virtual bool open(const MediaItemPtr media, const int offset = 0);

	/** �Đ� */
	void play();

	/** ��~ */
	void stop();

	bool useFastStop();

	/** �Đ������ǂ��� */
	const bool playing() const;

	const bool finished();

	/** �t�@�C�����N���[�Y���܂� */
	void close();

	/** �`��ȊO�̏��� */
	void process(const DWORD& frame);

	/** �X���b�h */
	virtual void run() = 0;

	/** �`�揈�� */
	void draw(const DWORD& frame);
};

typedef ComContent* ComContentPtr;