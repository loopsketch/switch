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


IEContent::IEContent(Renderer& renderer, int splitType, float x, float y, float w, float h): ComContent(renderer, splitType, x, y, w, h)
{
}

IEContent::~IEContent() {
}

void IEContent::createComComponents() {
	HRESULT hr = CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_INPROC_SERVER, IID_IOleObject, (void**)&_ole);
	if FAILED(hr) {
		_log.warning("failed create IOleObject");
	}

	_log.information("InternetExplorer initialized");
	_readCount = 0;
	_avgTime = 0;
	_phase = 1;
}

void IEContent::releaseComComponents() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	SAFE_RELEASE(_ole);
	_phase = 3;
	_log.information("flash released");
}

bool IEContent::open(const MediaItemPtr media, const int offset) {
	if (!_ole) return false;
	if (media->files().empty() || media->files().size() <= offset) return false;
	MediaItemFile mif = media->files()[offset];
	string url;
	if (mif.file().find("http://") == 0 || mif.file().find("https://") == 0) {
		url = mif.file();
	} else {
		url = Path(mif.file()).absolute(config().dataRoot).toString();
		Poco::File f(url);
		if (!f.exists()) {
			_log.warning(Poco::format("file not found: %s", url));
			return false;
		}
	}
	_url = url;

	return ComContent::open(media, offset);
}

void IEContent::run() {
	_log.information("start IE drawing thread");

	IOleClientSite* clientSite = NULL;
	HRESULT hr = _controlSite->QueryInterface(__uuidof(IOleClientSite), (void**)&clientSite);
	if FAILED(hr) {
		_log.warning("failed not query IOleClientSite");
		_phase = -1;
		return;
	}
	hr = _ole->SetClientSite(clientSite);
	if FAILED(hr) {
		_log.warning("failed not query IOleObject");
		clientSite->Release();	
		_phase = -1;
		return;
	}

	// Set the to transparent window mode
	IWebBrowser2* browser = NULL;
	hr = _ole->QueryInterface(IID_IWebBrowser2, (LPVOID*)&browser);
	if FAILED(hr) {
		_log.warning("failed IWebBrowser2");
		clientSite->Release();	
		_phase = -1;
		return;
	}

	wstring url;
	Poco::UnicodeConverter::toUTF16(_url, url);
	CComVariant empty;
	hr = browser->Navigate(_bstr_t(url.c_str()), &empty, &empty, &empty, &empty);
	if FAILED(hr) {
		_log.warning(Poco::format("failed not navigated: %s", _url));
		_phase = -1;
		return;
	}
	//long w, h;
	//_browser->get_Width(&w);
	//_browser->get_Height(&h);
	//_log.information(Poco::format("browser size: %ldx%ld", w, h));

	VARIANT_BOOL busy = VARIANT_FALSE;
	do {
		hr = browser->get_Busy(&busy);
		if FAILED(hr) {
			_log.warning("failed get_Busy");
			_phase = -1;
			return;
		}
		Sleep(100);
	} while (busy == VARIANT_TRUE);

	IDispatchPtr disp = NULL;
	hr = browser->get_Document(&disp);
	if FAILED(hr) {
		_log.warning("failed get_Document");
		_phase = -1;
		return;
	}
	IHTMLDocument2* doc = NULL;
	hr = disp->QueryInterface(IID_IHTMLDocument2, (void**)&doc);
	//hr = disp->QueryInterface(IID_IUnknown, (void**)&_doc);
	if FAILED(hr) {
		_log.warning("failed quey IHTMLDocument2");
		_phase = -1;
		return;
	}

	// In-place activate the object
	hr = _ole->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, clientSite, 0, NULL, NULL);
	clientSite->Release();	
		
	IOleInPlaceObjectWindowless* windowless = NULL;
	hr = _ole->QueryInterface(__uuidof(IOleInPlaceObjectWindowless), (LPVOID*)&windowless);
	if FAILED(hr) {
		_log.warning("failed not query IOleInPlaceObjectWindowless");
		_phase = -1;
		return;
	}

	IViewObject* view = NULL;
	hr = doc->QueryInterface(IID_IViewObject, (LPVOID*)&view);   
	if FAILED(hr) {
		_log.warning("failed not query IViewObject");
		_phase = -1;
		return;
	}
	SAFE_RELEASE(doc);
	IOleInPlaceObject* inPlaceObject = NULL;     
	hr = _ole->QueryInterface(__uuidof(IOleInPlaceObject), (LPVOID*) &inPlaceObject);
	if FAILED(hr) {
		_log.warning("failed not query IOleInPlaceObject");
		_phase = -1;
		return;
	}
	if (inPlaceObject != NULL) {
		RECT rect;
		SetRect(&rect, 0, 0, _w, _h);
		inPlaceObject->SetObjectRects(&rect, &rect);
		inPlaceObject->Release();
	}


	PerformanceTimer timer;
	while (_playing && _surface && view) {
		if (hasInvalidateRect()) {
			Rect rect = popInvalidateRect();
			timer.start();
			// _renderer.colorFill(_texture, 0x00000000);
			HDC hdc = NULL;
			HRESULT hr = _surface->GetDC(&hdc);
			if SUCCEEDED(hr) {
				SetMapMode(hdc, MM_TEXT);
				RECTL rectl = {rect.x, rect.y, rect.w, rect.h};
				hr = view->Draw(DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdc, NULL, &rectl, NULL, 0);
				if FAILED(hr) {
					_log.warning("failed drawing flash");
					break;
				}
				_surface->ReleaseDC(hdc);
				_readTime = timer.getTime();
				_readCount++;
				if (_readCount > 0) _avgTime = F(_avgTime * (_readCount - 1) + _readTime) / _readCount;
			} else {
				_log.warning("failed getDC");
			}
			Poco::Thread::sleep(0);
		} else {
			Poco::Thread::sleep(3);
		}
	}
	SAFE_RELEASE(browser);
	SAFE_RELEASE(view);
	SAFE_RELEASE(windowless);
	_log.information("finished IE drawing thread");
}
