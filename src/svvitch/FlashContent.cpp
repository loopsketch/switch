#include "FlashContent.h"
#include <Poco/UnicodeConverter.h>
//#include <Poco/DateTime.h>
//#include <Poco/Timezone.h>
#include "Utils.h"


FlashContent::FlashContent(Renderer& renderer, int splitType, float x, float y, float w, float h): Content(renderer, splitType, x, y, w, h),
	_phase(-1), _module(NULL), _classFactory(NULL),
	_controlSite(NULL), _ole(NULL), _flash(NULL), _windowless(NULL), _view(NULL),
	_texture(NULL), _surface(NULL), _playing(false), _zoom(0), _updated(false)
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
	char buf[MAX_PATH + 1];
	GetSystemDirectoryA(buf, MAX_PATH  + 1);
	string dir(buf);
	dir.append("\\macromed\\Flash\\");

	vector<string> files;
	files.push_back("flash10k.ocx");
	files.push_back("flash10j.ocx");
	files.push_back("flash10i.ocx");
	for (int i = 0; i < files.size(); i++) {
		string lib(dir + files[i]);
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
	//_module = LoadLibrary(L"C:\\WINDOWS\\SysWOW64\\macromed\\Flash\\flash10i.ocx");
	//_module = LoadLibrary(L"flash10e.ocx");
	if (!_module) {
		_log.warning("failed not loading flash ActiveX");
		return;
	}
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
	//flashInterface->PutQuality2("low");
	//flashInterface->PutQuality2("medium");
	//flashInterface->PutQuality2("high");
	//flashInterface->PutQuality2("best");
	//flashInterface->PutQuality2("autolow");
	//flashInterface->PutQuality2("autohigh");
	//_flash->put_Quality2(L"medium");

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
	if (!_params.empty()) {
		string params;
		svvitch::utf8_sjis(_params, params);
		_bstr_t bstr((char*)params.c_str());
		HRESULT hr = _flash->put_FlashVars(bstr);
		if SUCCEEDED(hr) {
			_log.information(Poco::format("flashvars: %s", _params));
		} else {
			_log.warning(Poco::format("failed flashvars: %s", _params));
		}
	}
	_log.information("flash initialized");
	_readCount = 0;
	_avgTime = 0;
	_phase = 1;
}

void FlashContent::releaseFlashComponents() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
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
	if (media->files().empty() || media->files().size() <= offset) return false;
	MediaItemFile mif = media->files()[offset];
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
	_movie = movie;
	_params = mif.params();
	_zoom = media->getNumProperty("swf_zoom", 0);
	_log.information(Poco::format("flash zoom: %d", _zoom));

	_controlSite = new ControlSite(this);
	_controlSite->AddRef();
	_texture = _renderer.createTexture(_w, _h, D3DFMT_X8R8G8B8);
	if (_texture) {
		_log.information(Poco::format("flash texture: %.0hfx%.0hf", _w, _h));
		_texture->GetSurfaceLevel(0, &_surface);
		_renderer.colorFill(_texture, 0xff000000);
	}

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
		_thread.join();
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
	switch (_phase) {
	case 2:
		if (_duration > 0) {
			//	return _flash->IsPlaying() == VARIANT_FALSE;
			//	return _current >= _duration;
			return _playTimer.getTime() >= 1000 * _duration / 60;
		}
		break;
	case 3:
		return true;
	}
	return false;
}

/** ファイルをクローズします */
void FlashContent::close() {
	stop();
	_mediaID.clear();
	SAFE_RELEASE(_surface);
	SAFE_RELEASE(_texture);
	SAFE_RELEASE(_controlSite);
	SAFE_RELEASE(_classFactory);
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
			string movie;
			svvitch::utf8_sjis(_movie, movie);
			_bstr_t bstr((char*)movie.c_str());
			//HRESULT hr = _flash->LoadMovie(0, bstr);
			HRESULT hr = _flash->put_Movie(bstr);
			if SUCCEEDED(hr) {
				_log.information(Poco::format("load movie: %s", _movie));
				_phase = 2;
				_flash->put_Scale(L"showAll");
				//_flash->put_Scale(L"exactfit");
				_worker = this;
				_thread.start(*_worker);

			} else {
				_log.warning(Poco::format("failed  movie: %s", _movie));
			}
		}
		break;
	case 2: // 再生中
		_current = _playTimer.getTime() * 60 / 1000;
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
	if (_duration > 0) {
		set("time_remain", t2);
	} else {
		set("time_remain", "--:--:--.--");
	}
	set("status", Poco::format("%03.2hfms", _avgTime));
}

void FlashContent::run() {
	_log.information("start flash drawing thread");
	PerformanceTimer timer;
	while (_playing && _surface && _view) {
		bool update = false;
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			if (_updated) {
				update = true;
				_updated = false;
			}
		}
		if (update) {
			timer.start();
			HDC hdc = NULL;
			HRESULT hr = _surface->GetDC(&hdc);
			if SUCCEEDED(hr) {
				_flash->Zoom(0);
				hr = _view->Draw(DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdc, NULL, NULL, NULL, 0);
				if FAILED(hr) _log.warning("failed drawing flash");
				_surface->ReleaseDC(hdc);
				_readTime = timer.getTime();
				_readCount++;
				_avgTime = F(_avgTime * (_readCount - 1) + _readTime) / _readCount;
			} else {
				_log.warning("failed getDC");
			}
			Poco::Thread::sleep(0);
		} else {
			Poco::Thread::sleep(10);
		}
	}
	_log.information("finished flash drawing thread");
}

void FlashContent::draw(const DWORD& frame) {
	switch (_phase) {
	case 2:
	case 3:
		if (_texture) {
			float alpha = getF("alpha");
			DWORD col = ((DWORD)(0xff * alpha) << 24) | 0xffffff;
			int cw = config().splitSize.cx;
			int ch = config().splitSize.cy;
			LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
			switch (config().splitType) {
			case 1:
				{
				}
				break;
			case 2:
				{
					//device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
					//RECT scissorRect;
					//device->GetScissorRect(&scissorRect);
					device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
					device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
					device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
					device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
					int sw = _w / cw;
					int sh = _h / ch;
					for (int sy = 0; sy < sh; sy++) {
						for (int sx = 0; sx < sw; sx++) {
							int dx = (sx / config().splitCycles) * cw;
							int dy = (config().splitCycles - (sx % config().splitCycles) - 1) * ch;
							//RECT rect = {dx, dy, dx + cww, dy + chh};
							//device->SetScissorRect(&rect);
							_renderer.drawTexture(dx + _x, dy + _y, cw, ch, sx * cw, sy * ch, cw, ch, _texture, 0, col, col, col, col);
						}
					}
					device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
					device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
					//device->SetScissorRect(&scissorRect);
				}
				break;
			default:
				_renderer.drawTexture(_x, _y, _texture, 0, col, col, col, col);
			}
		}
		break;
	}
}
