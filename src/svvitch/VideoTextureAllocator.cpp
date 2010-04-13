#include "VideoTextureAllocator.h"

VideoTextureAllocator::VideoTextureAllocator(Renderer& renderer): _log(Poco::Logger::get("")), _renderer(renderer) {
}

VideoTextureAllocator::~VideoTextureAllocator() {
}

// IVMRSurfaceAllocator9
HRESULT VideoTextureAllocator::InitializeDevice(DWORD_PTR dwUserID, VMR9AllocationInfo *lpAllocInfo, DWORD *lpNumBuffers) {
	return E_FAIL;
}

HRESULT VideoTextureAllocator::TerminateDevice(DWORD_PTR dwID) {
	return E_FAIL;
}

HRESULT VideoTextureAllocator::GetSurface(DWORD_PTR dwUserID, DWORD SurfaceIndex, DWORD SurfaceFlags, IDirect3DSurface9 **lplpSurface) {
	return E_FAIL;
}

HRESULT VideoTextureAllocator::AdviseNotify(IVMRSurfaceAllocatorNotify9* surfAllocNotify) {
	IDirect3D9 *d3d = NULL;
	_renderer.get3DDevice()->GetDirect3D(&d3d);
	HMONITOR monitor = d3d->GetAdapterMonitor(D3DADAPTER_DEFAULT);
	d3d->Release();
	HRESULT hr = surfAllocNotify->SetD3DDevice(_renderer.get3DDevice(), monitor);
	if (FAILED(hr)) {
		_log.information("failed SetD3DDevice()");
		return hr;
	}

//	LogoutInfo("TIAllocator AdviseNotify");
	return S_OK;
}


// IVMRImagePresenter9
 HRESULT VideoTextureAllocator::StartPresenting(DWORD_PTR userID) {
	return E_FAIL;
}

HRESULT VideoTextureAllocator::StopPresenting(DWORD_PTR userID) {
	return E_FAIL;
}

HRESULT VideoTextureAllocator::PresentImage(DWORD_PTR userID, VMR9PresentationInfo *lpPresInfo) {
	return E_FAIL;
}


// IVMRImageCompositor9
HRESULT VideoTextureAllocator::InitCompositionDevice(IUnknown* pD3DDevice) {
	return E_FAIL;
}

HRESULT VideoTextureAllocator::TermCompositionDevice(IUnknown* pD3DDevice) {
	return E_FAIL;
}

HRESULT VideoTextureAllocator::SetStreamMediaType(DWORD dwStrmID, AM_MEDIA_TYPE *pmt, BOOL fTexture) {
	return E_FAIL;
}

HRESULT VideoTextureAllocator::CompositeImage(IUnknown *pD3DDevice, IDirect3DSurface9 *pddsRenderTarget, AM_MEDIA_TYPE *pmtRenderTarget, REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd, D3DCOLOR dwClrBkGnd, VMR9VideoStreamInfo *pVideoStreamInfo, UINT cStreams) {
	return E_FAIL;
}


// IUnknown
HRESULT VideoTextureAllocator::QueryInterface(REFIID riid, void** ppvObject) {
    HRESULT hr = E_NOINTERFACE;

    if (ppvObject == NULL) {
        hr = E_POINTER;

	} else if (riid == IID_IVMRSurfaceAllocator9) {
        *ppvObject = static_cast<IVMRSurfaceAllocator9*>(this);
        AddRef();
        hr = S_OK;

	} else if (riid == IID_IVMRImagePresenter9) {
        *ppvObject = static_cast<IVMRImagePresenter9*>(this);
        AddRef();
        hr = S_OK;

	} else if (riid == IID_IVMRImageCompositor9) {
        *ppvObject = static_cast<IVMRImageCompositor9*>(this);
        AddRef();
        hr = S_OK;

	} else if (riid == IID_IUnknown) {
        *ppvObject = static_cast<IUnknown*>(static_cast<IVMRSurfaceAllocator9*>(this));
        AddRef();
        hr = S_OK;    
    }

    return hr;
}

ULONG VideoTextureAllocator::AddRef() {
    return InterlockedIncrement(&_refCount);
}

ULONG VideoTextureAllocator::Release() {
    ULONG ret = InterlockedDecrement(&_refCount);
	if (ret == 0) delete this;
    return ret;
}
