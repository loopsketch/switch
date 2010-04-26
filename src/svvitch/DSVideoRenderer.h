#pragma once

#include "Renderer.h"
#include <streams.h>
#include <dvdmedia.h>
#include <Poco/format.h>
#include <Poco/Logger.h>
#include <Poco/Mutex.h>
#include <queue>

using std::queue;


struct __declspec(uuid("{71771540-2017-11cf-ae26-0020afd79767}")) CLSID_DSVideoRenderer;


class DSVideoRenderer: public CBaseVideoRenderer {
private:
	Poco::Logger& _log;
	Poco::FastMutex _lock;
	Renderer& _renderer;
	D3DFORMAT _format;
	long _w;
	long _h;
	LPDIRECT3DTEXTURE9 _texture;

	PerformanceTimer _readTimer;
	DWORD _readTime;

	/** テクスチャを解放 */
	void releaseTexture();

	/** MediaTypeの情報を取得 */
	bool getMediaTypeName(const CMediaType* pmt, string& type, D3DFORMAT* format);

	/** YUY2→RGB変換処理 */
	void convertYUY2_RGB(LPBYTE dst, LPBYTE src, size_t len);

public:
	DSVideoRenderer(Renderer& renderer, LPUNKNOWN unk, HRESULT* result);
	virtual ~DSVideoRenderer();

	HRESULT CheckMediaType(const CMediaType* pmt);
	HRESULT SetMediaType(const CMediaType* pmt);
	HRESULT DoRenderSample(IMediaSample* sample);

	LPDIRECT3DTEXTURE9 getTexture();

	DWORD readTime();
};

typedef DSVideoRenderer* DSVideoRendererPtr;