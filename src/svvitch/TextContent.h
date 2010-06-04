#pragma once

#include "Content.h"

#include <gdiplus.h>


class TextContent: public Content {
private:
	Poco::FastMutex _lock;
	Poco::FastMutex _initializeLock;

	LPDIRECT3DTEXTURE9 _texture;
	TextContent* _referencedText;
	string _textFont;
	Gdiplus::FontStyle _textStyle;
	int _textHeight;
	Gdiplus::Color _c1;
	Gdiplus::Color _c2;
	Gdiplus::Color _b1;
	Gdiplus::REAL _borderSize1;
	Gdiplus::Color _b2;
	Gdiplus::REAL _borderSize2;
	int _ax;
	int _tw;
	int _th;
	int _iw;
	int _ih;
	int _cx;
	int _cy;
	int _cw;
	int _ch;
	string _move;
	float _dx;
	float _dy;
	string _align;
	Gdiplus::FontFamily _ff[16];

	void initialize();

	void drawText(string text, Gdiplus::Bitmap& bitmap, Gdiplus::Rect& rect);

public:
	TextContent(Renderer& renderer, float x = 0, float y = 0, float w = 0, float h = 0);

	virtual ~TextContent();

	bool open(const MediaItemPtr media, const int offset = 0);

	void setReference(TextContent* text);

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

typedef TextContent* TextContentPtr;