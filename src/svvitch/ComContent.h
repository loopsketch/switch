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
 * �L���v�`���[�V�[������L���v�`���[�f�����擾���`�悷��N���X
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


	ComContent(Renderer& renderer, int splitType, float x = 0, float y = 0, float w = 0, float h = 0);

	virtual ~ComContent();

	virtual void createComComponents() = 0;

	virtual void releaseComComponents() = 0;

	bool hasInvalidateRect();

	Rect popInvalidateRect();


public:
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

	void process(const DWORD& frame);

	virtual void run() = 0;

	void draw(const DWORD& frame);
};

typedef ComContent* ComContentPtr;