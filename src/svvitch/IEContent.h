#pragma once

#include <Exdisp.h>
#include "Content.h"


class IEContent: public Content {
private:
	Poco::FastMutex _lock;

	int _phase;
	string _url;
	IWebBrowser2* _browser;
	LPUNKNOWN _doc;
	IViewObject* _view;

	LPDIRECT3DTEXTURE9 _texture;
	LPDIRECT3DSURFACE9 _surface;


public:
	IEContent(Renderer& renderer, int splitType, float x = 0, float y = 0, float w = 0, float h = 0);

	virtual ~IEContent();

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

	const bool finished();

	/** �t�@�C�����N���[�Y���܂� */
	void close();

	void process(const DWORD& frame);

	void draw(const DWORD& frame);
};

typedef IEContent* IEContentPtr;