#pragma once

#include <streams.h>
#include "Renderer.h"
#include <vmr9.h>

class VideoTextureAllocator: public IVMRSurfaceAllocator9, IVMRImagePresenter9, public IVMRImageCompositor9
{
private:
	long _refCount;

	Poco::Logger& _log;
	Renderer& _renderer;


public:
	VideoTextureAllocator(Renderer& renderer);
	virtual ~VideoTextureAllocator();

	// IVMRSurfaceAllocator9
	virtual HRESULT STDMETHODCALLTYPE InitializeDevice(DWORD_PTR dwUserID, VMR9AllocationInfo *lpAllocInfo, DWORD *lpNumBuffers);

	virtual HRESULT STDMETHODCALLTYPE TerminateDevice(DWORD_PTR dwID);

	virtual HRESULT STDMETHODCALLTYPE GetSurface( DWORD_PTR dwUserID, DWORD SurfaceIndex, DWORD SurfaceFlags, IDirect3DSurface9 **lplpSurface);

	virtual HRESULT STDMETHODCALLTYPE AdviseNotify(IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify);

	// IVMRImagePresenter9
	virtual HRESULT STDMETHODCALLTYPE StartPresenting(DWORD_PTR dwUserID);

	virtual HRESULT STDMETHODCALLTYPE StopPresenting(DWORD_PTR dwUserID);

	virtual HRESULT STDMETHODCALLTYPE PresentImage(DWORD_PTR dwUserID, VMR9PresentationInfo *lpPresInfo);

	// IVMRImageCompositor9
	HRESULT STDMETHODCALLTYPE InitCompositionDevice(IUnknown* pD3DDevice);

	HRESULT STDMETHODCALLTYPE TermCompositionDevice(IUnknown* pD3DDevice);

	HRESULT STDMETHODCALLTYPE SetStreamMediaType(DWORD dwStrmID, AM_MEDIA_TYPE *pmt, BOOL fTexture);

	HRESULT STDMETHODCALLTYPE CompositeImage(IUnknown *pD3DDevice, IDirect3DSurface9 *pddsRenderTarget, AM_MEDIA_TYPE *pmtRenderTarget, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, D3DCOLOR dwClrBkGnd, VMR9VideoStreamInfo *pVideoStreamInfo, UINT cStreams);

	// IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);

	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
};

typedef VideoTextureAllocator* VideoTextureAllocatorPtr;
