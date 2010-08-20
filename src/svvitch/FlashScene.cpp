#ifdef USE_FLASH

#include "FlashScene.h"
#include <Poco/UnicodeConverter.h>



FlashScene::FlashScene(Renderer& renderer): Scene(renderer),
	_module(NULL), _controlSite(NULL), _ole(NULL), _flash(NULL), _windowless(NULL), _view(NULL),
	_buf(NULL)
{
}

FlashScene::~FlashScene() {
	SAFE_RELEASE(_view);
	SAFE_RELEASE(_windowless);
	SAFE_RELEASE(_flash);
	SAFE_RELEASE(_ole);
	SAFE_RELEASE(_controlSite);
	if (_module) FreeLibrary(_module);	
}

bool FlashScene::initialize() {
	_x = config().stageRect.left;
	_y = config().stageRect.top;
	_w = config().stageRect.right;
	_h = config().stageRect.bottom;
	_controlSite = new ControlSite();
	_controlSite->AddRef();	

	TCHAR buf[MAX_PATH+1];
	GetSystemDirectory(buf, MAX_PATH+1);
	wstring libw(buf);
	libw.append(L"\\macromed\\Flash\\flash10i.ocx");
	string lib;
	Poco::UnicodeConverter::toUTF8(libw, lib);
	_log.information(Poco::format("library: %s", lib));
	_module = LoadLibrary(libw.c_str());
	//_module = LoadLibrary(L"C:\\WINDOWS\\SysWOW64\\macromed\\Flash\\flash10i.ocx");
	//_module = LoadLibrary(L"flash10e.ocx");

	// Try the older version
	if (_module == NULL) {
		//_module = LoadLibrary(L"flash.ocx");
	}

	HRESULT hr;
	if (_module != NULL) {
		IClassFactory* pClassFactory = NULL;
		DllGetClassObjectFunc aDllGetClassObjectFunc = (DllGetClassObjectFunc) GetProcAddress(_module, "DllGetClassObject");
		aDllGetClassObjectFunc(CLSID_ShockwaveFlash, IID_IClassFactory, (void**)&pClassFactory);
		if (pClassFactory) {
			hr = pClassFactory->CreateInstance(NULL, IID_IOleObject, (void**)&_ole);
			pClassFactory->Release();
			if FAILED(hr) {
				_log.warning("failed create IOleObject");
				return false;
			}
		} else {
			_log.warning("failed create IOleObject");
			return false;
		}
	} else {
		hr = CoCreateInstance(CLSID_ShockwaveFlash, NULL, CLSCTX_INPROC_SERVER, IID_IOleObject, (void**)&_ole);
		if FAILED(hr) {
			_log.warning("failed create IOleObject");
			return false;
		}
	}

	IOleClientSite* clientSite = NULL;
	_controlSite->QueryInterface(__uuidof(IOleClientSite), (void**) &clientSite);
	hr = _ole->SetClientSite(clientSite);
	if FAILED(hr) {
		_log.warning("failed query IOleObject");
		return false;
	}

	// Set the to transparent window mode
	hr = _ole->QueryInterface(__uuidof(IShockwaveFlash), (LPVOID*) &_flash);
	if FAILED(hr) {
		_log.warning("failed IShockwaveFlash");
		return false;
	}
	_flash->put_WMode(L"transparent");

	// In-place activate the object
	hr = _ole->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, clientSite, 0, NULL, NULL);
	clientSite->Release();	
		
	hr = _ole->QueryInterface(__uuidof(IOleInPlaceObjectWindowless), (LPVOID*) &_windowless);
	if FAILED(hr) {
		_log.warning("failed quey IOleInPlaceObjectWindowless");
		return false;
	}

	hr = _flash->QueryInterface(IID_IViewObject, (LPVOID*) &_view);   
	if FAILED(hr) {
		_log.warning("failed quey IViewObject");
		return false;
	}
	_buf = _renderer.createTexture(_w, _h, D3DFMT_A8R8G8B8);
	IOleInPlaceObject* inPlaceObject = NULL;     
	_ole->QueryInterface(__uuidof(IOleInPlaceObject), (LPVOID*) &inPlaceObject);
	if (inPlaceObject != NULL) {
		RECT rect;
		SetRect(&rect, _x, _y, _w, _h);
		inPlaceObject->SetObjectRects(&rect, &rect);
		inPlaceObject->Release();
	}
	_log.information("flash initialized");
	return true;
}

void FlashScene::process() {
	if (_flash) {
		//Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		if (!_movie.empty()) {
			_bstr_t bstr((char*)_movie.c_str());           
			HRESULT hr = _flash->put_Movie(bstr);
			if SUCCEEDED(hr) {
				_log.information(Poco::format("movie: %s", _movie));
			} else {
				_log.warning(Poco::format("failed  movie: %s", _movie));
			}
			_movie.clear();
		}
		if (_buf) {
			LPDIRECT3DSURFACE9 surface = NULL;
			_buf->GetSurfaceLevel(0, &surface);
			if (surface) {
				HDC hdc = NULL;
				HRESULT hr = surface->GetDC(&hdc);
				if SUCCEEDED(hr) {
					if (_view != NULL) {
						//_renderer.colorFill(_buf, 0xff000000);
						// RECT is relative to the windowless container rect
						RECTL rectl = {L(0), L(0), L(_w), L(_h)};
						//HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
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
}

void FlashScene::draw1() {
	if (_buf) {
		DWORD col = 0xffffffff;
		_renderer.drawTexture(_x, _y, _buf, 0, col, col, col, col);
	}
}

void FlashScene::draw2() {
	if (_flash) {
		long state = getReadyState();
		long frame = getCurrentFrame();
		string playing(isPlaying()?"play":"stop");
		_renderer.drawFontTextureText(0, 0, 10, 10, 0xccff3333, Poco::format("swf state:%ld frame:%ld [%s]", state, frame, playing));
	} else {
		_renderer.drawFontTextureText(0, 0, 10, 10, 0xccff3333, "flash not ready");
	}
}


long FlashScene::getReadyState() {
	long state = -1;
	if (_flash) _flash->get_ReadyState(&state);
	return state;
}

bool FlashScene::loadMovie(const string& file) {
	_movie = file;
	return true;
}

bool FlashScene::isPlaying() {
	VARIANT_BOOL isPlaying = 0;
	if (_flash) _flash->get_Playing(&isPlaying);
	return (isPlaying != 0);
}

int FlashScene::getCurrentFrame() {
	long currentFrame = -1;
	if (_flash) _flash->get_FrameNum(&currentFrame);
	return currentFrame;
}

#endif