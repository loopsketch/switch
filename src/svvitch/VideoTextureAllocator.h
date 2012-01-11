#pragma once

#include <streams.h>
#include "Renderer.h"
#include <vmr9.h>
#include <Dvdmedia.h>

#include <Poco/Mutex.h>


/**
 * ビデオテクスチャアロケータクラス.
 * VMR-9で使用するカスタムアロケータです
 */
class VideoTextureAllocator: public IVMRSurfaceAllocator9, IVMRImagePresenter9, public IVMRImageCompositor9
{
private:
	Poco::Logger& _log;
	long _refCount;
	Renderer& _renderer;
	Poco::FastMutex _lock;
	LPDIRECT3DTEXTURE9 _texture;
	int _w;
	int _h;
	D3DFORMAT _format;
	bool _presenting;

public:
	VideoTextureAllocator(Renderer& renderer);
	virtual ~VideoTextureAllocator();

	/** テクスチャ取得 */
	LPDIRECT3DTEXTURE9 getTexture();

	float getDisplayAspectRatio();

	// IVMRSurfaceAllocator9
	/** デバイスの初期化 */
	virtual HRESULT STDMETHODCALLTYPE InitializeDevice(DWORD_PTR userID, VMR9AllocationInfo* info, DWORD* buffers);

	/** デバイスの解放 */
	virtual HRESULT STDMETHODCALLTYPE TerminateDevice(DWORD_PTR userID);

	virtual HRESULT STDMETHODCALLTYPE GetSurface(DWORD_PTR userID, DWORD index, DWORD SurfaceFlags, LPDIRECT3DSURFACE9* surface);

	virtual HRESULT STDMETHODCALLTYPE AdviseNotify(IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify);

	// IVMRImagePresenter9
	/** ビデオの再生前 */
	virtual HRESULT STDMETHODCALLTYPE StartPresenting(DWORD_PTR dwUserID);

	/** ビデオの停止直後 */
	virtual HRESULT STDMETHODCALLTYPE StopPresenting(DWORD_PTR dwUserID);

	virtual HRESULT STDMETHODCALLTYPE PresentImage(DWORD_PTR dwUserID, VMR9PresentationInfo *lpPresInfo);

	// IVMRImageCompositor9
	HRESULT STDMETHODCALLTYPE InitCompositionDevice(IUnknown* pD3DDevice);

	HRESULT STDMETHODCALLTYPE TermCompositionDevice(IUnknown* pD3DDevice);

	HRESULT STDMETHODCALLTYPE SetStreamMediaType(DWORD streamID, AM_MEDIA_TYPE* pmt, BOOL fTexture);

	HRESULT STDMETHODCALLTYPE CompositeImage(IUnknown *pD3DDevice, IDirect3DSurface9 *pddsRenderTarget, AM_MEDIA_TYPE *pmtRenderTarget, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, D3DCOLOR dwClrBkGnd, VMR9VideoStreamInfo *pVideoStreamInfo, UINT cStreams);

	// IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);

	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
};

typedef VideoTextureAllocator* VideoTextureAllocatorPtr;
