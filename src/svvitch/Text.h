#pragma once

#include "Content.h"

#include <gdiplus.h>


class Text: public Content {
private:
	Poco::FastMutex _lock;
	Poco::FastMutex _initializeLock;

	LPDIRECT3DTEXTURE9 _texture;
	string _font;
	int _fontH;
	int _tw;
	int _th;
	int _iw;
	int _ih;
	int _cx;
	int _cy;
	int _cw;
	int _ch;
	float _dx;
	float _dy;
	Gdiplus::FontFamily _ff[16];

	void drawText(string text, Gdiplus::Bitmap& bitmap, Gdiplus::Rect& rect);

public:
	Text(Renderer& renderer, float x = 0, float y = 0, float w = 0, float h = 0);

	~Text();

	void initialize();

	bool open(const MediaItemPtr media, const int offset = 0);

	void play();

	void stop();

	const bool finished();

	/** ファイルをクローズします */
	void close();

	void drawTexture(string text);

	/** 1フレームに1度だけ処理される */
	void process(const DWORD& frame);

	/** 描画 */
	void draw(const DWORD& frame);
};

typedef Text* TextPtr;