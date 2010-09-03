#ifdef USE_FLASH


#include "FlashContent.h"
#include <Poco/UnicodeConverter.h>
//#include <Poco/DateTime.h>
//#include <Poco/Timezone.h>
#include "Utils.h"


FlashContent::FlashContent(Renderer& renderer, float x, float y, float w, float h): Content(renderer, x, y, w, h),
	_phase(-1), _module(NULL), _controlSite(NULL), _ole(NULL), _flash(NULL), _windowless(NULL), _view(NULL),
	_texture(NULL), _playing(false)
{
	initialize();
}

FlashContent::~FlashContent() {
	close();
	if (_module) {
		FreeLibrary(_module);
		_module = NULL;
	}
}

void FlashContent::initialize() {
	_log.information("flash initialize");
	char buf[MAX_PATH + 1];
	GetSystemDirectoryA(buf, MAX_PATH  + 1);
	string lib(buf);
	lib.append("\\macromed\\Flash\\flash10i.ocx");
	_log.information(Poco::format("library: %s", lib));
	_module = LoadLibraryA(lib.c_str());
	//_module = LoadLibrary(L"C:\\WINDOWS\\SysWOW64\\macromed\\Flash\\flash10i.ocx");
	//_module = LoadLibrary(L"flash10e.ocx");

	// Try the older version
	if (_module == NULL) {
		//_module = LoadLibrary(L"flash.ocx");
		_log.warning("failed not load 'flash.ocx'");
	}
	_phase = 0;
}

void FlashContent::createFlashComponents() {
	HRESULT hr;
	if (_module) {
		IClassFactory* pClassFactory = NULL;
		DllGetClassObjectFunc aDllGetClassObjectFunc = (DllGetClassObjectFunc) GetProcAddress(_module, "DllGetClassObject");
		aDllGetClassObjectFunc(CLSID_ShockwaveFlash, IID_IClassFactory, (void**)&pClassFactory);
		if (pClassFactory) {
			hr = pClassFactory->CreateInstance(NULL, IID_IOleObject, (void**)&_ole);
			pClassFactory->Release();
			if FAILED(hr) {
				_log.warning("failed create IOleObject");
				_phase = -1;
				return;
			}
			_log.information("created class ShockwaveFlash(LoadLibrary)");
		} else {
			_log.warning("failed create IOleObject");
			_phase = -1;
			return;
		}
	} else {
		hr = CoCreateInstance(CLSID_ShockwaveFlash, NULL, CLSCTX_INPROC_SERVER, IID_IOleObject, (void**)&_ole);
		if FAILED(hr) {
			_log.warning("failed create IOleObject");
			_phase = -1;
			return;
		}
		_log.information("created class ShockwaveFlash");
	}

	_controlSite = new ControlSite();
	_controlSite->AddRef();

	IOleClientSite* clientSite = NULL;
	hr = _controlSite->QueryInterface(__uuidof(IOleClientSite), (void**) &clientSite);
	if FAILED(hr) {
		_log.warning("failed query IOleClientSite");
		_phase = -1;
		return;
	}
	hr = _ole->SetClientSite(clientSite);
	if FAILED(hr) {
		_log.warning("failed query IOleObject");
		_phase = -1;
		return;
	}

	// Set the to transparent window mode
	hr = _ole->QueryInterface(__uuidof(IShockwaveFlash), (LPVOID*) &_flash);
	if FAILED(hr) {
		_log.warning("failed IShockwaveFlash");
		_phase = -1;
		return;
	}
	_flash->put_WMode(L"transparent");

	// In-place activate the object
	hr = _ole->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, clientSite, 0, NULL, NULL);
	clientSite->Release();	
		
	hr = _ole->QueryInterface(__uuidof(IOleInPlaceObjectWindowless), (LPVOID*) &_windowless);
	if FAILED(hr) {
		_log.warning("failed quey IOleInPlaceObjectWindowless");
		_phase = -1;
		return;
	}

	hr = _flash->QueryInterface(IID_IViewObject, (LPVOID*) &_view);   
	if FAILED(hr) {
		_log.warning("failed quey IViewObject");
		_phase = -1;
		return;
	}
	_texture = _renderer.createTexture(_w, _h, D3DFMT_X8R8G8B8);
	IOleInPlaceObject* inPlaceObject = NULL;     
	_ole->QueryInterface(__uuidof(IOleInPlaceObject), (LPVOID*) &inPlaceObject);
	if (inPlaceObject != NULL) {
		RECT rect;
		SetRect(&rect, 0, 0, _w, _h);
		inPlaceObject->SetObjectRects(&rect, &rect);
		inPlaceObject->Release();
	}
	_flash->put_Loop(VARIANT_FALSE);
	_log.information("flash initialized");
	_phase = 1;
}

void FlashContent::releaseFlashComponents() {
	_log.information("flash release");
	SAFE_RELEASE(_texture);
	SAFE_RELEASE(_view);
	SAFE_RELEASE(_windowless);
	SAFE_RELEASE(_flash);
	SAFE_RELEASE(_ole);
	SAFE_RELEASE(_controlSite);
	_phase = 4;
	_log.information("flash released");
}

/** ファイルをオープンします */
bool FlashContent::open(const MediaItemPtr media, const int offset) {
	//Poco::ScopedLock<Poco::FastMutex> lock(_lock);

	//load the movie
	//_log.information(Poco::format("ready state before: %ld", _flash->ReadyState));
	MediaItemFile mif = media->files()[0];
	if (mif.file().find("http://") == 0) {
		_movie = mif.file();
	} else {
		//file = "file://" + Path(mif.file()).absolute(config().dataRoot).toString(Poco::Path::PATH_UNIX);
		_movie = Path(mif.file()).absolute(config().dataRoot).toString();
	}

	set("alpha", 1.0f);
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
	_log.information("flash play");
}

/**
 * 停止
 */
void FlashContent::stop() {
	_playing = false;
	_log.information("flash stop");
	if (_phase >= 0) {
		releaseFlashComponents();
	}
}

bool FlashContent::useFastStop() {
	return false;
}

/**
 * 再生中かどうか
 */
const bool FlashContent::playing() const {
	return _playing;
}

const bool FlashContent::finished() {
	if (_playing) {
	//	return _flash->IsPlaying() == VARIANT_FALSE;
		return _current >= _duration;
	}
	return false;
}

/** ファイルをクローズします */
void FlashContent::close() {
	stop();
	_mediaID.clear();
}

void FlashContent::process(const DWORD& frame) {
	switch (_phase) {
	case 0: // 初期化フェーズ
		if (_playing) {
			createFlashComponents();
		}
		break;
	case 1: // 初期化済
		if (_playing && !_movie.empty()) {
			_bstr_t bstr((char*)_movie.c_str());
			HRESULT hr = _flash->raw_LoadMovie(0, bstr);
			if SUCCEEDED(hr) {
				_log.information(Poco::format("movie: %s", _movie));
			} else {
				_log.warning(Poco::format("failed  movie: %s", _movie));
			}
			_phase = 2;
		}
		break;
	case 2: // 再生中
		if (_playing && _texture) {
			LPDIRECT3DSURFACE9 surface = NULL;
			_texture->GetSurfaceLevel(0, &surface);
			if (surface) {
				HDC hdc = NULL;
				HRESULT hr = surface->GetDC(&hdc);
				if SUCCEEDED(hr) {
					if (_view != NULL) {
						//RECT rect;
						//_controlSite->GetRect(&rect);
						//RECTL rectl = {rect.left, rect.top, rect.right, rect.bottom};
						RECTL rectl = {0, 0, _w, _h};
						hr = _view->Draw(DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdc, &rectl, NULL, NULL, 0);
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
			_current++;
		}
		break;
	case 3: // クローズ
		break;
	case 4: // クローズ終了
		break;
	}
}

void FlashContent::draw(const DWORD& frame) {
	switch (_phase) {
	case 2:
		if (_playing && _texture) {
			float alpha = getF("alpha");
			DWORD col = ((DWORD)(0xff * alpha) << 24) | 0xffffff;
			_renderer.drawTexture(_x, _y, _texture, 0, col, col, col, col);
		}
		break;
	}
}

#endif
