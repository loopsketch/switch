#include "IEContent.h"
#ifdef UNICODE
#define FormatMessage FormatMessageW
#define FindResource FindResourceW
#else
#define FormatMessage FormatMessageA
#define FindResource FindResourceA
#endif // !UNICODE
#include <comdef.h>
#include <atlcomcli.h>
#include <comutil.h>
#include <mshtml.h>
#include <Poco/UnicodeConverter.h>
#include "Utils.h"


IEContent::IEContent(Renderer& renderer, int splitType, float x, float y, float w, float h): Content(renderer, splitType, x, y, w, h),
	_browser(NULL), _doc(NULL), _view(NULL), _texture(NULL), _surface(NULL)
{
	CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_SERVER, IID_IWebBrowser2, (LPVOID*)&_browser);
}

IEContent::~IEContent() {
	SAFE_RELEASE(_view);
	SAFE_RELEASE(_browser);
	SAFE_RELEASE(_surface);
	SAFE_RELEASE(_texture);
}

bool IEContent::open(const MediaItemPtr media, const int offset) {
	if (!_browser) return false;
	if (media->files().empty() || media->files().size() <= offset) return false;
	MediaItemFile mif = media->files()[offset];

	wstring url;
	Poco::UnicodeConverter::toUTF16(mif.file(), url);
	CComVariant empty;
	HRESULT hr = _browser->Navigate(_bstr_t(url.c_str()), &empty, &empty, &empty, &empty);
	if FAILED(hr) {
		_log.warning(Poco::format("failed not navigated: %s", mif.file()));
		return false;
	}
	long w, h;
	_browser->get_Width(&w);
	_browser->get_Height(&h);
	_log.information(Poco::format("browser size: %ldx%ld", w, h));

	VARIANT_BOOL busy = VARIANT_FALSE;
	do {
		hr = _browser->get_Busy(&busy);
		if FAILED(hr) {
			_log.warning("failed get_Busy");
			return false;
		}
		Sleep(100);
	} while (busy == VARIANT_TRUE);

	IDispatchPtr disp;
	hr = _browser->get_Document(&disp);
	if FAILED(hr) {
		_log.warning("failed get_Document");
		return false;
	}
	IHTMLDocument2* doc = NULL;
	hr = disp->QueryInterface(IID_IHTMLDocument2, (void**)&doc);
	//hr = disp->QueryInterface(IID_IUnknown, (void**)&_doc);
	if FAILED(hr) {
		_log.warning("failed quey IHTMLDocument2");
		return false;
	}
	hr = doc->QueryInterface(IID_IViewObject, (LPVOID*)&_view);
	if FAILED(hr) {
		_log.warning("failed quey IViewObject");
		return false;
	}
	SAFE_RELEASE(doc);

	_texture = _renderer.createTexture(_w, _h, D3DFMT_X8R8G8B8);
	if (_texture) {
		_log.information(Poco::format("browser texture: %.0hfx%.0hf", _w, _h));
		_texture->GetSurfaceLevel(0, &_surface);
	}
	_mediaID = media->id();
	return true;
}

void IEContent::play() {
	_playing = true;
}

void IEContent::stop() {
	_playing = false;
}

const bool IEContent::finished() {
	return !_playing;
}

void IEContent::close() {
	if (_browser) _browser->Quit();
}

void IEContent::process(const DWORD& frame) {
	if (_playing && _view) {
		HDC hdc = NULL;
		HRESULT hr = _surface->GetDC(&hdc);
		if SUCCEEDED(hr) {
			RECT rect = {0, 0, _w, _h};
			hr = OleDraw(_view, DVASPECT_CONTENT, hdc, &rect);
			//hr = _view->Draw(DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdc, NULL, NULL, NULL, 0);
			if FAILED(hr) _log.warning("failed drawing browser");
			_surface->ReleaseDC(hdc);
		} else {
			_log.warning("failed getDC");
		}
	}
}

void IEContent::draw(const DWORD& frame) {
	if (_playing) {
		DWORD col = 0xffffffff;
		_renderer.drawTexture(_x, _y, _texture, 0, col, col, col, col);
	}
}
