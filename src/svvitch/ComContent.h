#pragma once

#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>

#include "Content.h"
#include "ControlSite.h"


using std::queue;

/**
 * 領域クラス.
 * 領域値のホルダ
 */
class Rect
{
private:
	/** 領域のコピー処理 */
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
 * COM(ActiveX)のコンテントクラス.
 * COMを描画するクラスです
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

	/** コンストラクタ */
	ComContent(Renderer& renderer, int splitType, float x = 0, float y = 0, float w = 0, float h = 0);

	/** デストラクタ */
	virtual ~ComContent();

	/** COM生成 */
	virtual void createComComponents() = 0;

	/** COM開放 */
	virtual void releaseComComponents() = 0;

	/** invalidate領域があるか */
	bool hasInvalidateRect();

	/** invalidate領域を取出します */
	Rect popInvalidateRect();


public:
	/** invalidate領域を通知します */
	void invalidateRect(int x, int y, int w, int h);

	/** ファイルをオープンします */
	virtual bool open(const MediaItemPtr media, const int offset = 0);

	/** 再生 */
	void play();

	/** 停止 */
	void stop();

	bool useFastStop();

	/** 再生中かどうか */
	const bool playing() const;

	const bool finished();

	/** ファイルをクローズします */
	void close();

	/** 描画以外の処理 */
	void process(const DWORD& frame);

	/** スレッド */
	virtual void run() = 0;

	/** 描画処理 */
	void draw(const DWORD& frame);
};

typedef ComContent* ComContentPtr;