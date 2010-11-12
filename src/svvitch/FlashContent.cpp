#include "FlashContent.h"
#include <Poco/UnicodeConverter.h>
//#include <Poco/DateTime.h>
//#include <Poco/Timezone.h>
#include "Utils.h"


FlashContent::FlashContent(Renderer& renderer, int splitType, float x, float y, float w, float h): ComContent(renderer, splitType, x, y, w, h),
	_module(NULL), _classFactory(NULL), _zoom(0)
{
	initialize();
}

FlashContent::~FlashContent() {
	close();
	SAFE_RELEASE(_classFactory);
	if (_module) {
		FreeLibrary(_module);
		_module = NULL;
	}
}

void FlashContent::initialize() {
	char buf[MAX_PATH + 1];
	GetSystemDirectoryA(buf, MAX_PATH  + 1);
	string dir(buf);
	dir.append("\\macromed\\Flash\\");

	vector<string> files;
	files.push_back("flash10l.ocx");
	files.push_back("flash10k.ocx");
	files.push_back("flash10j.ocx");
	files.push_back("flash10i.ocx");
	files.push_back("flash10i.ocx");
	files.push_back("flash9.ocx");
	for (int i = 0; i < files.size(); i++) {
		string lib(dir + files[i]);
		Poco::File f(lib);
		if (f.exists()) {
			_module = LoadLibraryA(lib.c_str());
			if (_module) {
				DllGetClassObjectFunc aDllGetClassObjectFunc = (DllGetClassObjectFunc) GetProcAddress(_module, "DllGetClassObject");
				aDllGetClassObjectFunc(CLSID_ShockwaveFlash, IID_IClassFactory, (void**)&_classFactory);
				if (!_classFactory) {
					FreeLibrary(_module);
					_module = NULL;
				} else {
					_log.information(Poco::format("load library: %s", lib));
					break;
				}
			}
		}
	}

	if (!_module) {
		_log.warning("failed not loading flash ActiveX");
		return;
	}
	_phase = 0;
}

void FlashContent::createComComponents() {
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

	_log.information("flash initialized");
	_readCount = 0;
	_avgTime = 0;
	_phase = 1;
}

void FlashContent::releaseComComponents() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	SAFE_RELEASE(_ole);
	_phase = 3;
	_log.information("flash released");
}

/** ファイルをオープンします */
bool FlashContent::open(const MediaItemPtr media, const int offset) {
	if (!_module) return false;

	//load the movie
	//_log.information(Poco::format("ready state before: %ld", _flash->ReadyState));
	if (media->files().empty() || media->files().size() <= offset) return false;
	MediaItemFile mif = media->files()[offset];
	string movie;
	if (mif.file().find("http://") == 0 || mif.file().find("https://") == 0) {
		movie = mif.file();
	} else {
		movie = Path(mif.file()).absolute(config().dataRoot).toString();
		Poco::File f(movie);
		if (!f.exists()) {
			_log.warning(Poco::format("file not found: %s", movie));
			return false;
		}
	}
	_movie = movie;
	_params = mif.params();
	_zoom = media->getNumProperty("swf_zoom", 0);
	_log.information(Poco::format("flash zoom: %d", _zoom));
	_background = media->getHexProperty("swf_background", 0);
	_log.information(Poco::format("flash background: %lx", _background));

	return ComContent::open(media, offset);
}

void FlashContent::run() {
	_log.information("start flash drawing thread");

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
	IShockwaveFlash* flash = NULL;
	hr = _ole->QueryInterface(__uuidof(IShockwaveFlash), (LPVOID*)&flash);
	if FAILED(hr) {
		_log.warning("failed IShockwaveFlash");
		clientSite->Release();	
		_phase = -1;
		return;
	}
	flash->put_WMode(L"transparent"); // opaque
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
	hr = flash->QueryInterface(IID_IViewObject, (LPVOID*)&view);   
	if FAILED(hr) {
		_log.warning("failed not query IViewObject");
		_phase = -1;
		return;
	}
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

	flash->put_Loop(VARIANT_FALSE);
	if (!_params.empty()) {
		string params;
		svvitch::utf8_sjis(_params, params);
		_bstr_t bstr((char*)params.c_str());
		HRESULT hr = flash->put_FlashVars(bstr);
		if SUCCEEDED(hr) {
			_log.information(Poco::format("flashvars: %s", _params));
		} else {
			_log.warning(Poco::format("failed flashvars: %s", _params));
		}
	}

	string movie;
	svvitch::utf8_sjis(_movie, movie);
	//HRESULT hr = flash->LoadMovie(0, bstr);
	hr = flash->put_Movie(_bstr_t(movie.c_str()));
	if SUCCEEDED(hr) {
		_log.information(Poco::format("load movie: %s", _movie));
		// flash->PutQuality2("low");
		// flash->PutQuality2("medium");
		// flash->PutQuality2("high");
		// flash->PutQuality2("best");
		// flash->PutQuality2("autolow");
		// flash->PutQuality2("autohigh");
		// flash->put_Quality2(L"medium");
		//flash->put_BackgroundColor(_background);
		//flash->put_Scale(L"exactfit");
		flash->put_Scale(L"showAll");
		flash->Zoom(_zoom);
	} else {
		_log.warning(Poco::format("failed movie: %s", _movie));
		_worker = NULL;
		_phase = -1;
		return;
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
	SAFE_RELEASE(flash);
	SAFE_RELEASE(view);
	SAFE_RELEASE(windowless);
	_log.information("finished flash drawing thread");
}
