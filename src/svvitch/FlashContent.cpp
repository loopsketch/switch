#include "FlashContent.h"
#include <Poco/UnicodeConverter.h>
//#include <Poco/DateTime.h>
//#include <Poco/Timezone.h>
#include "Utils.h"


FlashContent::FlashContent(Renderer& renderer, float x, float y, float w, float h): Content(renderer, x, y, w, h),
	_phase(-1), _module(NULL), _classFactory(NULL),
	_controlSite(NULL), _ole(NULL), _flash(NULL), _windowless(NULL), _view(NULL),
	_texture(NULL), _playing(false), _updated(false)
{
	initialize();
}

FlashContent::~FlashContent() {
	close();
	SAFE_RELEASE(_texture);
	SAFE_RELEASE(_controlSite);
	SAFE_RELEASE(_classFactory);
	if (_module) {
		FreeLibrary(_module);
		_module = NULL;
	}
}

void FlashContent::initialize() {
	char buf[MAX_PATH + 1];
	GetSystemDirectoryA(buf, MAX_PATH  + 1);
	string lib(buf);
	lib.append("\\macromed\\Flash\\flash10i.ocx");
	_module = LoadLibraryA(lib.c_str());
	if (_module) {
		_log.information(Poco::format("library: %s", lib));
		DllGetClassObjectFunc aDllGetClassObjectFunc = (DllGetClassObjectFunc) GetProcAddress(_module, "DllGetClassObject");
		aDllGetClassObjectFunc(CLSID_ShockwaveFlash, IID_IClassFactory, (void**)&_classFactory);
		if (!_classFactory) {
			FreeLibrary(_module);
			_module = NULL;
		}
	}
	//_module = LoadLibrary(L"C:\\WINDOWS\\SysWOW64\\macromed\\Flash\\flash10i.ocx");
	//_module = LoadLibrary(L"flash10e.ocx");
	if (!_module) {
		_log.warning("failed not loading flash ActiveX");
		return;
	}
	_controlSite = new ControlSite(this);
	_controlSite->AddRef();
	_texture = _renderer.createTexture(_w, _h, D3DFMT_X8R8G8B8);
	_renderer.colorFill(_texture, 0xff000000);
	_phase = 0;
}

void FlashContent::createFlashComponents() {
	HRESULT hr;
	if (_module) {
		if (_classFactory) {
			hr = _classFactory->CreateInstance(NULL, IID_IOleObject, (void**)&_ole);
			SAFE_RELEASE(_classFactory);
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
		clientSite->Release();	
		_phase = -1;
		return;
	}

	// Set the to transparent window mode
	hr = _ole->QueryInterface(__uuidof(IShockwaveFlash), (LPVOID*) &_flash);
	if FAILED(hr) {
		_log.warning("failed IShockwaveFlash");
		clientSite->Release();	
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
	SAFE_RELEASE(_view);
	SAFE_RELEASE(_windowless);
	SAFE_RELEASE(_flash);
	SAFE_RELEASE(_ole);
	_log.information("flash released");
	_phase = 3;
}

/** ファイルをオープンします */
bool FlashContent::open(const MediaItemPtr media, const int offset) {
	if (!_module) return false;
	//Poco::ScopedLock<Poco::FastMutex> lock(_lock);

	//load the movie
	//_log.information(Poco::format("ready state before: %ld", _flash->ReadyState));
	MediaItemFile mif = media->files()[0];
	string movie;
	if (mif.file().find("http://") == 0) {
		movie = mif.file();
	} else {
		movie = Path(mif.file()).absolute(config().dataRoot).toString();
		Poco::File f(movie);
		if (!f.exists()) {
			_log.warning(Poco::format("file not found: %s", movie));
			return false;
		}
	}
	svvitch::utf8_sjis(movie, _movie);
	//_movie = movie;

	set("alpha", 1.0f);
	_duration = media->duration() * 60 / 1000;
	_current = 0;
	_mediaID = media->id();
	return true;
}

void FlashContent::update() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	_updated = true;
}

/**
 * 再生
 */
void FlashContent::play() {
	_playing = true;
	_playTimer.start();
}

/**
 * 停止
 */
void FlashContent::stop() {
	_playing = false;
	if (_phase >= 0) {
		releaseFlashComponents();
	}
}

bool FlashContent::useFastStop() {
	return true;
}

/**
 * 再生中かどうか
 */
const bool FlashContent::playing() const {
	return _playing;
}

const bool FlashContent::finished() {
	if (_phase == 2) {
	//	return _flash->IsPlaying() == VARIANT_FALSE;
	//	return _current >= _duration;
		return _playTimer.getTime() >= 1000 * _duration / 60;
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
			string movie;
			svvitch::sjis_utf8(_movie, movie);
			if SUCCEEDED(hr) {
				_log.information(Poco::format("load movie: %s", movie));
				_phase = 2;
			} else {
				_log.warning(Poco::format("failed  movie: %s", movie));
			}
		}
		break;
	case 2: // 再生中
		if (_playing && _texture) {
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			if (_updated) {
				_updated = false;
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
			}
			_current = _playTimer.getTime() * 60 / 1000;
		}
		break;
	case 3: // リリース済
		break;
	}
	unsigned long cu = _playTimer.getTime() / 1000;
	unsigned long re = _duration / 60 - cu;
	string t1 = Poco::format("%02lu:%02lu:%02lu.%02d", cu / 3600, cu / 60, cu % 60, 0);
	string t2 = Poco::format("%02lu:%02lu:%02lu.%02d", re / 3600, re / 60, re % 60, 0);
	set("time", Poco::format("%s %s", t1, t2));
	set("time_current", t1);
	set("time_remain", t2);
}

void FlashContent::draw(const DWORD& frame) {
	switch (_phase) {
	case 2:
	case 3:
		if (_texture) {
			float alpha = getF("alpha");
			DWORD col = ((DWORD)(0xff * alpha) << 24) | 0xffffff;
			_renderer.drawTexture(_x, _y, _texture, 0, col, col, col, col);
		}
		break;
	}
}
