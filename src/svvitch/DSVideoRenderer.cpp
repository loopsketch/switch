#include "DSVideoRenderer.h"
#include <Dvdmedia.h>


#define WRITE_CLIPPED_BYTE(D, S)  { int v = S; if (v < 0) v = 0; if (v > 255) v = 255; (D)=(BYTE)v; }


DSVideoRenderer::DSVideoRenderer(Renderer& renderer, LPUNKNOWN unk, HRESULT* result):
	CBaseVideoRenderer(__uuidof(CLSID_DSVideoRenderer), NAME("DSVideoRenderer"), unk, result),
	_log(Poco::Logger::get("")), _renderer(renderer), _w(0), _h(0), _texture(NULL), _readTime(0)
{
	AddRef();
	_format = D3DFMT_UNKNOWN;
	*result = S_OK;

}

DSVideoRenderer::~DSVideoRenderer() {
	releaseTexture();
	_log.information("*release video renderer");
}

void DSVideoRenderer::releaseTexture() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	SAFE_RELEASE(_texture);
}

bool DSVideoRenderer::getMediaTypeName(const CMediaType* pmt, string& type, D3DFORMAT* format) {
	bool result = false;
	if (IsEqualGUID(*pmt->Type(), MEDIATYPE_Video)) {
		if (IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_MPEG1Packet)) {
			type = "MPEG1-Packet";
			result = false;
		} else if (IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_MPEG1Payload)) {
			type = "MPEG1-Payload";
			result = false;
		} else if (IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_RGB24)) {
			type = "RGB24";
			*format = D3DFMT_R8G8B8;
			result = false;
		} else if (IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_RGB32)) {
			type = "RGB32";
			*format = D3DFMT_A8R8G8B8;
			result = true;
		} else if (IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_RGB555)) {
			type = "RGB555";
			*format = D3DFMT_X1R5G5B5;
			result = false;
		} else if (IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_RGB565)) {
			type = "RGB565";
			*format = D3DFMT_R5G6B5;
			result = false;
		} else if (IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_RGB8)) {
			type = "RGB8";
			*format = D3DFMT_A8R8G8B8;
			result = false;


		} else if (IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_YUY2)) { // Packed YUYV
			type = "YUY2";
			*format = D3DFMT_YUY2;
			result = false;
		} else if (IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_YUYV)) { // Packed
			type = "YUYV";
			*format = D3DFMT_UNKNOWN;
			result = false;
		} else if (IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_UYVY)) { // Packed
			type = "UYVY";
			*format = D3DFMT_UNKNOWN;
			result = false;
		} else if (IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_Y41P)) { // Packed
			type = "Y41P";
			*format = D3DFMT_UNKNOWN;
			result = false;


		} else if (IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_YV12)) { // Planer YVU
			type = "YV12";
			*format = D3DFMT_UNKNOWN;
			result = false;

		} else {
			string serial;
			for (int i = 0; i < 8; i++) {
				serial += Poco::format(",%02?x", pmt->Subtype()->Data4[i]);
			}
			string guid = Poco::format("unknown[%08?x,%04?x,%04?x%s]", pmt->Subtype()->Data1, pmt->Subtype()->Data2, pmt->Subtype()->Data3, serial);
			_log.warning(Poco::format("invalid media subtype: %s", guid));
			//Poco::format("unknown type: %lx-%lx-%lx-%lx", pmt->Subtype()->Data1, pmt->Subtype()->Data2, pmt->Subtype()->Data3, pmt->Subtype()->Data4);
		}
	}
	return result;
}

HRESULT DSVideoRenderer::CheckMediaType(const CMediaType* pmt) {
	CheckPointer(pmt, E_POINTER);
	if (!IsEqualGUID(*pmt->Type(), MEDIATYPE_Video)) {
		// 
		string serial;
		for (int i = 0; i < 8; i++) {
			serial += Poco::format(",%02?x", pmt->FormatType()->Data4[i]);
		}
		string guid = Poco::format("unknown[%08?x,%04?x,%04?x%s]", pmt->FormatType()->Data1, pmt->FormatType()->Data2, pmt->FormatType()->Data3, serial);
		_log.warning(Poco::format("invalid format type: %s", guid));
		return E_INVALIDARG;
	}

	HRESULT hr = E_FAIL;
	long w = 0;
	long h = 0;
	int i = 0;
	string type;
	D3DFORMAT format;
	if (IsEqualGUID(*pmt->FormatType(), FORMAT_VideoInfo)) {
		VIDEOINFOHEADER* info = (VIDEOINFOHEADER*)pmt->Format();
		w = info->bmiHeader.biWidth;
		h = info->bmiHeader.biHeight;
		i = 1;
	} else if (IsEqualGUID(*pmt->FormatType(), FORMAT_VideoInfo2)) {
		VIDEOINFOHEADER2* info = (VIDEOINFOHEADER2*)pmt->Format();
		w = info->bmiHeader.biWidth;
		h = info->bmiHeader.biHeight;
		i = 2;
	} else if (IsEqualGUID(*pmt->FormatType(), FORMAT_MPEGVideo)) {
		MPEG1VIDEOINFO* info = (MPEG1VIDEOINFO*)pmt->Format();
		w = info->hdr.bmiHeader.biWidth;
		h = info->hdr.bmiHeader.biHeight;
		i = 3;
	}
	if (getMediaTypeName(pmt, type, &format)) {
		hr = S_OK;
	}
	_log.information(Poco::format("check media type[%d]: %s %ldx%ld", i, type, w, h));
	return hr;
}

HRESULT DSVideoRenderer::SetMediaType(const CMediaType* pmt) {
	HRESULT hr = E_FAIL;
	VIDEOINFO* info = (VIDEOINFO*)pmt->Format();
	long w = info->bmiHeader.biWidth;
	long h = info->bmiHeader.biHeight;
	if (w != 0 && h != 0) {
		releaseTexture();
		string type;
		D3DFORMAT format;
		if (getMediaTypeName(pmt, type, &format)) {
			switch (format) {
				case D3DFMT_R8G8B8:
					_format = format;
					_w = w;
					_h = h;
					_texture = _renderer.createTexture(_w, _h, D3DFMT_X8R8G8B8);
					if (_texture) hr = S_OK;
					break;
				case D3DFMT_X8R8G8B8:
				case D3DFMT_A8R8G8B8:
					_format = format;
					_w = w;
					_h = h;
					_texture = _renderer.createTexture(_w, _h, _format);
					if (_texture) hr = S_OK;
					break;
				case D3DFMT_YUY2:
					_format = format;
					_w = w;
					_h = h;
					_texture = _renderer.createTexture(_w, _h, D3DFMT_X8R8G8B8);
					if (_texture) hr = S_OK;
					break;
			}
		}
		_log.information(Poco::format("set media type: %s %ldx%ld", type, w, h));
	}
	return hr;
}

HRESULT DSVideoRenderer::DoRenderSample(IMediaSample* sample) {
	CheckPointer(sample, E_POINTER);
	CheckPointer(_texture, E_UNEXPECTED);

	HRESULT hr = E_FAIL;
	AM_MEDIA_TYPE* type;
	sample->GetMediaType(&type);
	if (type) {
		_log.information("*change media type");
		DeleteMediaType(type);
	}

	LPBYTE src, dst;
	sample->GetPointer(&src);
	long size = sample->GetActualDataLength();
	if (_texture) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		_readTimer.start();
		D3DLOCKED_RECT lockRect = {0};
		if (SUCCEEDED(_texture->LockRect(0, &lockRect, 0, 0))) {
			dst = (LPBYTE)lockRect.pBits;
			switch (_format) {
				case D3DFMT_R8G8B8:
					break;
				case D3DFMT_X8R8G8B8:
				case D3DFMT_A8R8G8B8:
					CopyMemory(dst, src, size);
					break;
				case D3DFMT_YUY2:
					convertYUY2_RGB(dst, src, size);
					break;
			}
			_texture->UnlockRect(0);
			hr = S_OK;
		} else {
			_log.warning("failed capture texture lock");
		}
		_readTime = _readTimer.getTime();
//		timeBeginPeriod(1);
//		Sleep(1);
//		timeEndPeriod(1);
	} else {
		hr = E_UNEXPECTED;
	}
	return hr;
}

LPDIRECT3DTEXTURE9 DSVideoRenderer::getTexture() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	return _texture;
}

DWORD DSVideoRenderer::readTime() {
	return _readTime;
}

/**
 * YUY2→RGB変換処理を行います
 * dst	描画先(byte*)
 * src	ベースフィールドのソース(byte*)
 **/
void DSVideoRenderer::convertYUY2_RGB(LPBYTE dst, LPBYTE src, size_t len) {
	long pos = 0;
	int Y0, Y1, U, V;
	for (int i = 0; i < len; i += 4) {
		Y0 = 298 * (src[i + 0] - 16);
		Y1 = 298 * (src[i + 2] - 16);
		U  = src[i + 1] - 128;
		V  = src[i + 3] - 128;

		WRITE_CLIPPED_BYTE(dst[pos++], (Y0 + 516 * U           + 128) >> 8); // B
		WRITE_CLIPPED_BYTE(dst[pos++], (Y0 - 100 * U - 208 * V + 128) >> 8); // G
		WRITE_CLIPPED_BYTE(dst[pos++], (Y0           + 409 * V + 128) >> 8); // R
//		dst[pos++] = 0xff;
		pos++;

		WRITE_CLIPPED_BYTE(dst[pos++], (Y1 + 516 * U           + 128) >> 8); // B
		WRITE_CLIPPED_BYTE(dst[pos++], (Y1 - 100 * U - 208 * V + 128) >> 8); // G
		WRITE_CLIPPED_BYTE(dst[pos++], (Y1           + 409 * V + 128) >> 8); // R
		pos++;
//		dst[pos++] = 0xff;
	}
}
