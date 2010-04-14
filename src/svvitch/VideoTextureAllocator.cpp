#include "VideoTextureAllocator.h"
#include <Poco/format.h>
#include <Poco/UUID.h>

VideoTextureAllocator::VideoTextureAllocator(Renderer& renderer): _log(Poco::Logger::get("")), _refCount(0), _renderer(renderer), _texture(NULL) {
}

VideoTextureAllocator::~VideoTextureAllocator() {
}

LPDIRECT3DTEXTURE9 VideoTextureAllocator::getTexture() {
	return _texture;
}

// IVMRSurfaceAllocator9
HRESULT VideoTextureAllocator::InitializeDevice(DWORD_PTR userID, VMR9AllocationInfo* info, DWORD* buffers) {
	if (buffers == NULL) return E_POINTER;

	//_log.information(Poco::format("** initialize device %lu %lu", userID, ((DWORD)info->Format)));
	string s = Poco::format("aspect:%ld/%ld native:%ldx%ld", info->szAspectRatio.cx, info->szAspectRatio.cy, info->szNativeSize.cx, info->szNativeSize.cy);
	_log.information(Poco::format("** initialize device(%lu) format:%lu %lux%lu(%s) x %lu", userID, ((DWORD)info->Format), info->dwWidth, info->dwHeight, s, *buffers));
	D3DFORMAT format = D3DFMT_X8R8G8B8;
	LPDIRECT3DTEXTURE9 texture = NULL;
	switch (info->dwFlags) {
		case VMR9AllocFlag_3DRenderTarget:
			texture = _renderer.createRenderTarget(info->dwWidth, info->dwHeight, format);
			break;
		default:
			_log.warning(Poco::format("failed not initialize flag: %lu", info->dwFlags));
	}
	if (texture) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		SAFE_RELEASE(_texture);
		_texture = texture;
		return S_OK;
	}
	return E_FAIL;
}

HRESULT VideoTextureAllocator::TerminateDevice(DWORD_PTR userID) {
	_log.information("** terminate device");
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	SAFE_RELEASE(_texture);
	return S_OK;
}

HRESULT VideoTextureAllocator::GetSurface(DWORD_PTR userID, DWORD index, DWORD surfaceFlags, LPDIRECT3DSURFACE9* surface) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (!_texture) return E_FAIL;
	if (surface == NULL) {
		_log.warning(Poco::format("no surface: %lu", userID));
		return E_POINTER;
	}

	if (index >= 1) {
		_log.warning(Poco::format("outOfIndex:%lu flags:%lu", userID, surfaceFlags));
		Sleep(5000);
		return E_FAIL;
	}

	HRESULT hr = E_FAIL;
	hr = _texture->GetSurfaceLevel(0, surface);
	if FAILED(hr) {
		_log.warning("** failed get surface");
		return hr;
	}

	//_log.information("** get surfaced");

	return hr;
}

HRESULT VideoTextureAllocator::AdviseNotify(IVMRSurfaceAllocatorNotify9* surfAllocNotify) {
	_log.information("** advise notify");
	IDirect3D9 *d3d = NULL;
	_renderer.get3DDevice()->GetDirect3D(&d3d);
	HMONITOR monitor = d3d->GetAdapterMonitor(D3DADAPTER_DEFAULT);
	d3d->Release();
	HRESULT hr = surfAllocNotify->SetD3DDevice(_renderer.get3DDevice(), monitor);
	if (FAILED(hr)) {
		_log.information("failed SetD3DDevice()");
		return hr;
	}
	return S_OK;
}


// IVMRImagePresenter9
 HRESULT VideoTextureAllocator::StartPresenting(DWORD_PTR userID) {
	_log.information("** start presenting");
	return S_OK;
}

HRESULT VideoTextureAllocator::StopPresenting(DWORD_PTR userID) {
	_log.information("** stop presenting");
	return S_OK;
}

HRESULT VideoTextureAllocator::PresentImage(DWORD_PTR userID, VMR9PresentationInfo *info) {
	// レンダリング可能状態
	return S_OK;
}


// IVMRImageCompositor9
HRESULT VideoTextureAllocator::InitCompositionDevice(IUnknown* pD3DDevice) {
	_log.information("** init composite device");
	return S_OK;
}

HRESULT VideoTextureAllocator::TermCompositionDevice(IUnknown* pD3DDevice) {
	_log.information("** term composite device");
	return S_OK;
}

HRESULT VideoTextureAllocator::SetStreamMediaType(DWORD streamID, AM_MEDIA_TYPE* pmt, BOOL fTexture) {
	if (pmt == NULL) {
		_w = 0;
		_h = 0;
		return S_OK;
	}

	if (pmt->majortype != MEDIATYPE_Video) {
		_log.warning("** failed AM_MEDIA_TYPE not video");
		return E_FAIL;
	}

	string formatType;
	string subType;
	long w = 0;
	long h = 0;
	LONGLONG tpf = 0;
	if (pmt->formattype == FORMAT_VideoInfo) {
		VIDEOINFOHEADER* vih = reinterpret_cast<VIDEOINFOHEADER*>(pmt->pbFormat);
		//_isInterlaced = false;
		tpf = vih->AvgTimePerFrame;
		w = vih->bmiHeader.biWidth;
		h = vih->bmiHeader.biHeight;
		if (h < 0) h = -h;
		formatType = "VideoInfo";
	} else if (pmt->formattype == FORMAT_VideoInfo2) {
		VIDEOINFOHEADER2* vih = reinterpret_cast<VIDEOINFOHEADER2*>(pmt->pbFormat);
		//_isInterlaced = (vih->dwInterlaceFlags & AMINTERLACE_IsInterlaced) != 0;
		tpf = vih->AvgTimePerFrame;
		w = vih->bmiHeader.biWidth;
		h = vih->bmiHeader.biHeight;
		if (h < 0) h = -h;
		formatType = "VideoInfo2";
	}

	D3DFORMAT format = D3DFMT_X8R8G8B8;
	HRESULT hr = E_FAIL;
	if (pmt->subtype == MEDIASUBTYPE_YUY2 || pmt->subtype == MEDIASUBTYPE_YUYV) {
		subType = "YUY2/YUYV";
		//_timePerFrames = tpf;
		format = D3DFMT_YUY2;
		hr = S_OK;
	} else if (pmt->subtype == MEDIASUBTYPE_IYUV) {
		subType = "IYUV";
		//_timePerFrames = tpf;
		format = D3DFMT_UYVY;
		// hr = S_OK;
	} else if (pmt->subtype == MEDIASUBTYPE_Y41P) {
		subType = "Y41P";
		//_timePerFrames = tpf;
		format = D3DFMT_UYVY;
		// hr = S_OK;
	} else if (pmt->subtype == MEDIASUBTYPE_UYVY) {
		subType = "UYVY";
		//_timePerFrames = tpf;
		format = D3DFMT_UYVY;
		hr = S_OK;
	} else if (pmt->subtype == MEDIASUBTYPE_YVYU) {
		subType = "YVYU";
		//_timePerFrames = tpf;
		format = D3DFMT_UYVY;
		hr = S_OK;
	} else if (pmt->subtype == MEDIASUBTYPE_RGB565) {
		subType = "RGB565";
		//_timePerFrames = tpf;
		format = D3DFMT_R5G6B5;
		hr = S_OK;
	} else if (pmt->subtype == MEDIASUBTYPE_RGB555) {
		subType = "RGB555";
		//_timePerFrames = tpf;
		format = D3DFMT_X1R5G5B5;
		hr = S_OK;
	} else if (pmt->subtype == MEDIASUBTYPE_RGB24) {
		subType = "RGB24";
		//_timePerFrames = tpf;
		format = D3DFMT_R8G8B8;
		hr = S_OK;
	} else if (pmt->subtype == MEDIASUBTYPE_RGB32) {
		subType = "RGB32";
		format = D3DFMT_X8R8G8B8;
		//_timePerFrames = tpf;
		hr = S_OK;
	} else if (pmt->subtype == MEDIASUBTYPE_ARGB32) {
		subType = "ARGB32";
		//_timePerFrames = tpf;
		format = D3DFMT_A8R8G8B8;
		hr = S_OK;
	} else {
		// UuidToString(&pmt->subtype, (unsigned short**)&subType);
		string serial;
		for (int i = 0; i < 8; i++) {
			serial += Poco::format(",%02?x", pmt->subtype.Data4[i]);
		}
		subType = Poco::format("unknown[%08?x,%04?x,%04?x%s]", pmt->subtype.Data1, pmt->subtype.Data2, pmt->subtype.Data3, serial);
		//_timePerFrames = tpf;
		// hr = S_OK;
	}
	_log.information(Poco::format("allocator format[%s] stream[%s] %ldx%ld(%.2hf)", formatType, subType, w, h, (F(10000000) / tpf)));

	if (SUCCEEDED(hr) && (_w != w || _h != h || _format != format) && _h == 0) {
		// サイズ変化があり、_h=0 ならテクスチャを再生成します
		LPDIRECT3DTEXTURE9 texture = _renderer.createRenderTarget(w, h, format);
		if (texture) {
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			SAFE_RELEASE(_texture);
			_texture = texture;
			_log.information(Poco::format("size/format changed, create texture %dx%d %x", w, h, format));
		}
	}
	_w = w;
	_h = h;
	_format = format;
	return hr;
}

HRESULT VideoTextureAllocator::CompositeImage(IUnknown* pD3DDevice, IDirect3DSurface9* rt, AM_MEDIA_TYPE* pmt, REFERENCE_TIME start, REFERENCE_TIME end, D3DCOLOR background, VMR9VideoStreamInfo* info, UINT streams) {
	HRESULT hr = _renderer.get3DDevice()->StretchRect(info->pddsVideoSurface, NULL, rt, NULL, D3DTEXF_NONE);
	return S_OK;
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
