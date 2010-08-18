#ifdef USE_FLASH

#define _ATL_APARTMENT_THREADED
//#define _ATL_FREE_THREADED
//#define _ATL_NO_AUTOMATIC_NAMESPACE
//#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS

//#include <windows.h>
#include <atlbase.h>
//#include <atlbase.h>
//#include <atlwin.h>
#include "FlashContent.h"
#include <Poco/UnicodeConverter.h>
//#include <Poco/DateTime.h>
//#include <Poco/Timezone.h>
#include "Utils.h"


FlashContent::FlashContent(Renderer& renderer, float x, float y, float w, float h): Content(renderer, x, y, w, h),
	_module(NULL), _controlSite(NULL), _ole(NULL), _flash(NULL), _windowless(NULL), _view(NULL),
	_buf(NULL)
{
	initialize();
}

FlashContent::~FlashContent() {
	close();
	SAFE_RELEASE(_view);
	SAFE_RELEASE(_windowless);
	SAFE_RELEASE(_flash);
	SAFE_RELEASE(_ole);
	SAFE_RELEASE(_controlSite);
	if (_module) FreeLibrary(_module);	
}

void FlashContent::initialize() {
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	_controlSite = new ControlSite();
	//_controlSite->Init(this);

	//HRESULT hr;
	_module = LoadLibraryA("C:\\WINDOWS\\system32\\macromed\\Flash\\flash10i.ocx");
	if (_module == NULL) {
		//_module = LoadLibraryA("flash.ocx");
	}
	if (_module != NULL) {
		IClassFactory* classFactory = NULL;
		DllGetClassObjectFunc aDllGetClassObjectFunc = (DllGetClassObjectFunc)GetProcAddress(_module, "DllGetClassObject");
		hr = aDllGetClassObjectFunc(CLSID_ShockwaveFlash, IID_IClassFactory, (void**)&classFactory);
		classFactory->CreateInstance(NULL, IID_IOleObject, (void**)&_ole);
		classFactory->Release();
	} else {
		// 元々CLSCTX_ALL CLSCTX_INPROC_SERVER CLSID_ShockwaveFlash
		hr = CoCreateInstance(CLSID_ShockwaveFlash, NULL, CLSCTX_INPROC_SERVER, IID_IOleObject, (void**)&_ole);
		if FAILED(hr) {
			_log.warning("failed not created 'ShockwaveFlash' class");
			return;
		}
	}

	IOleClientSite* clientSite = NULL;
	hr = _controlSite->QueryInterface(__uuidof(IOleClientSite), (void**)&clientSite);
	if FAILED(hr) {
		_log.warning("failed not query 'IOleClientSite'");
		return;
	}
	hr = _ole->SetClientSite(clientSite);
	if FAILED(hr) {
		_log.warning("failed SetClientSite");
		return;
	}

	hr = _ole->QueryInterface(__uuidof(IShockwaveFlash), (LPVOID*)&_flash);
	if FAILED(hr) {
		_log.warning("failed not query 'IShockwaveFlash'");
		return;
	}
	_flash->DisableLocalSecurity();
	_flash->PutEmbedMovie(FALSE);
	_flash->PutAllowScriptAccess(L"always");
	_flash->PutLoop(VARIANT_TRUE);
	_flash->put_Quality2(L"High");
	long ver = _flash->FlashVersion();
	_log.information(Poco::format("flash version: %f", (ver / 65536.0)));
	// Transparent opaque
	_flash->PutWMode(L"transparent");

	hr = _ole->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, clientSite, 0, NULL, NULL);
	if FAILED(hr) {
		_log.warning("failed DoVerb");
		//return;
	}
	clientSite->Release();

	hr = _ole->QueryInterface(__uuidof(IOleInPlaceObjectWindowless), (LPVOID*)&_windowless);
	if FAILED(hr) {
		_log.warning("failed not query 'IOleInPlaceObjectWindowless'");
		//return;
	}
	hr = _flash->QueryInterface(IID_IViewObject, (LPVOID*)&_view);
	if FAILED(hr) {
		_log.warning("failed not query 'IViewObject'");
		return;
	}
	_log.information("flash initialized");
}

/** ファイルをオープンします */
bool FlashContent::open(const MediaItemPtr media, const int offset) {
	//Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (!_flash) return false;

	//load the movie
	//_log.information(Poco::format("ready state before: %ld", _flash->ReadyState));
	MediaItemFile mif = media->files()[0];
	string file;
	if (mif.file().find("http://") == 0) {
		file = mif.file();
	} else {
		//file = "file://" + Path(mif.file()).absolute(config().dataRoot).toString(Poco::Path::PATH_UNIX);
		file = Path(mif.file()).absolute(config().dataRoot).toString();
	}
	_log.information(Poco::format("movie: %s", file));
	//string sjis;
	//svvitch::utf8_sjis(file, sjis);
	//wstring wfile;
	//Poco::UnicodeConverter::toUTF16(file, wfile);
	HRESULT hr = _flash->put_Movie(_bstr_t(file.c_str()));   
	if (FAILED(hr)) {
		_log.warning(Poco::format("failed movie: %s", file));
		return false;
	}
	_buf = _renderer.createTexture(L(_w), L(_h), D3DFMT_A8R8G8B8);
	IOleInPlaceObject* inPlaceObject = NULL;
	hr = _ole->QueryInterface(__uuidof(IOleInPlaceObject), (LPVOID*)&inPlaceObject);
	if SUCCEEDED(hr) {
		RECT rect;
		::SetRect(&rect, 0, 0, _w, _h);
		hr = inPlaceObject->SetObjectRects(&rect, &rect);
		if (FAILED(hr)) {
			_log.warning("failed SetObjectRects");
		}
		inPlaceObject->Release();
	} else {
		_log.warning("failed query 'IOleInPlaceObject'");
		return false;
	}

	set("alpha", 1.0f);
	//_duration = _flash->GetTotalFrames();
	//_flash->StopPlay();
	_duration = media->duration() * 60 / 1000;
	_current = 0;
	_mediaID = media->id();
	return true;
}


/**
 * 再生
 */
void FlashContent::play() {
	_playing = true;
	if (_flash) {
		//HRESULT hr = _flash->Play();
	}
}

/**
 * 停止
 */
void FlashContent::stop() {
	_playing = false;
	if (_flash) {
		//HRESULT hr = _flash->Stop();
	}
}

bool FlashContent::useFastStop() {
	return false;
}

/**
 * 再生中かどうか
 */
const bool FlashContent::playing() const {
	if (_flash && _playing) {
		return _flash->IsPlaying() == VARIANT_TRUE;
	}
	return _playing;
}

const bool FlashContent::finished() {
	//if (_flash && _playing) {
	//	return _flash->IsPlaying() == VARIANT_FALSE;
	//}
	return false;
	//return _current >= _duration;
}

/** ファイルをクローズします */
void FlashContent::close() {
	stop();
	_mediaID.clear();
	SAFE_RELEASE(_buf);
}

void FlashContent::process(const DWORD& frame) {
	if (_flash && _buf) {
		LPDIRECT3DSURFACE9 surface = NULL;
		_buf->GetSurfaceLevel(0, &surface);
		if (surface) {
			HDC hdc = NULL;
			HRESULT hr = surface->GetDC(&hdc);
			if SUCCEEDED(hr) {
				if (_view != NULL) {
					// RECT is relative to the windowless container rect
					RECTL rectl = {L(0), L(0), L(_w), L(_h)};
					//HBRUSH brush = CreateSolidBrush(RGB(100, 100, 110));
					//::FillRect(hdc, (RECT*)&rectl, brush);
					hr = _view->Draw(DVASPECT_CONTENT, 1, NULL, NULL, NULL, hdc, &rectl, NULL, NULL, 0);
					if FAILED(hr) _log.warning("failed draw");
					//DeleteObject(brush);
				}
				surface->ReleaseDC(hdc);
			} else {
				_log.warning("failed getDC");
			}
			surface->Release();
		} else {
			_log.warning("failed get surface");
		}
	}
}

void FlashContent::draw(const DWORD& frame) {
	if (_flash) {
		if (_playing && _buf) {
			DWORD col = 0xffffffff;
			_renderer.drawTexture(_x, _y, _buf, 0, col, col, col, col);
		}

		long state = _flash->GetReadyState();
		long frame = _flash->CurrentFrame();
		string playing((_flash->IsPlaying() == VARIANT_TRUE)?"play":"stop");
		_renderer.drawFontTextureText(0, 0, 10, 10, 0xccff3333, Poco::format("swf state:%ld frame:%03ld [%s]", state, frame, playing));
	}
}

#endif
