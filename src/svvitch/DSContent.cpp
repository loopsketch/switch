#include "DSContent.h"
#include <Poco/UnicodeConverter.h>


DSContent::DSContent(Renderer& renderer): Content(renderer), _gb(NULL), _vmr9(NULL), _allocator(NULL), _vr(NULL), _mc(NULL), _ms(NULL), _me(NULL)
{
}

DSContent::~DSContent() {
	initialize();
}

void DSContent::initialize() {
	close();
}

/** ファイルをオープンします */
bool DSContent::open(const MediaItemPtr media, const int offset) {
	initialize();
	if (media->files().empty()) return false;

	HRESULT hr;
	//hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		_log.warning(Poco::format("failed CoInitializeEx: hr=0x%lx", ((unsigned long)hr)));
		return false;
	}
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&_gb);
	if (FAILED(hr)) {
		_log.warning(Poco::format("failed create filter graph: hr=0x%lx", ((unsigned long)hr)));
		return false;
	}

	if (false) {
		hr = CoCreateInstance(CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&_vmr9);
		if (FAILED(hr)) {
			//SAFE_RELEASE(s->gb);
			//TCHAR err[1024];
			//DWORD res = AMGetErrorText(hr, err, 1024);
			//_log.warning(L"failed creation VideoMixingRenderer9: [%s]", err);
			_log.warning("failed creation VideoMixingRenderer9");
			return false;
		}

		IVMRFilterConfig9 *fc;
		hr = _vmr9->QueryInterface(&fc);
		if (FAILED(hr)) {
			_log.warning("VMR-9 setup failed(VMRFilterConfig9 not found)");
			return false;
		}

		hr = fc->SetRenderingMode(VMR9Mode_Renderless);
		if (FAILED(hr)) {
			_log.warning("VMR-9 setup failed(failed set renderless)");
			return false;
		}
		hr = fc->SetNumberOfStreams(1);
		if (FAILED(hr)) {
			_log.warning("VMR-9 setup failed(failed set streams)");
			return false;
		}

		IVMRSurfaceAllocatorNotify9 *allocatorNotify;
		_vmr9->QueryInterface(&allocatorNotify);
		if (FAILED(hr)) {
			SAFE_RELEASE(fc);
			_log.warning("failed surface allocator not created");
			return false;
		}

		// VMR-9にカスタムアロケータを通知
		_allocator = new VideoTextureAllocator(_renderer);
		// s->allocator->setSurfacePrepareInterval(1);
		hr = allocatorNotify->AdviseSurfaceAllocator(0, _allocator);
		if (FAILED(hr)) {
			SAFE_RELEASE(allocatorNotify);
			SAFE_RELEASE(fc);
			_log.warning("failed advise surface allocator");
			return false;
		}

		// アロケータにVMR-9に登録したことを通知
		hr = _allocator->AdviseNotify(allocatorNotify);
		if (FAILED(hr)) {
			SAFE_RELEASE(allocatorNotify);
			SAFE_RELEASE(fc);
			_log.warning("failed not advised notify surface allocator");
			return false;
		}

		//hr = fc->SetImageCompositor(_allocator);
		//if (FAILED(hr)) {
		//	SAFE_RELEASE(allocatorNotify);
		//	SAFE_RELEASE(fc);
		//	_log.warning("***ImageCompositor creation failed");
		//	return false;
		//}
		SAFE_RELEASE(allocatorNotify);
		SAFE_RELEASE(fc);

		hr = _gb->AddFilter(_vmr9, L"VMR-9");
		if (FAILED(hr)) {
			_log.warning("failed not add VMR-9");
			return false;
		}



	} else {
		_vr = new DSVideoRenderer(_renderer, NULL, &hr);
		if (FAILED(hr = _gb->AddFilter(_vr, L"DSVideoRenderer"))) {
			_log.warning(Poco::format("failed add filter: hr=0x%lx", hr));
			return false;
		}
	}

	MediaItemFile mif = media->files()[0];
	wstring wfile;
	Poco::UnicodeConverter::toUTF16(Path(mif.file()).absolute(config().dataRoot).toString(), wfile);
	IBaseFilter* src = NULL;
	hr = _gb->AddSourceFilter(wfile.c_str(), L"File Source", &src);
	if (SUCCEEDED(hr) && src) {
		IPin* srcOut = NULL;
		if (getOutPin(src, &srcOut)) {
			hr = _gb->Render(srcOut);
			SAFE_RELEASE(srcOut);
			if SUCCEEDED(hr) {
				while (getOutPin(src, &srcOut)) {
					// ソースから複数の出力ピンが出てる場合を考慮…
					hr = _gb->Render(srcOut);
					SAFE_RELEASE(srcOut);
					if (FAILED(hr)) {
						_log.warning("multi src out pin rendering failed");
						break;
					}
				}
			} else {
				_log.warning("failed render outpin");
			}
		}
	} else if (hr == VFW_E_NOT_FOUND) {
		_log.warning(Poco::format("source not found: %s", mif.file()));
	} else {
		_log.warning(Poco::format("failed add source filter: %s", mif.file()));
	}
	SAFE_RELEASE(src);
	if FAILED(hr) {
		return false;
	}
	if (dumpFilter(_gb) > 0) {
		_log.warning("failed lookup another video renderer");
		return false;
	}

	hr = _gb->QueryInterface(&_mc);
	if (FAILED(hr)) {
		_log.warning("failed query interface: IMediaControl");
		return false;
	}	
	hr = _gb->QueryInterface(&_ms);
	if (FAILED(hr)) {
		_log.warning("failed query interface: IMediaSeeking");
		return false;
	}
	hr = _gb->QueryInterface(&_me);
	if (FAILED(hr)) {
		_log.warning("failed query interface: IMediaEvent");
		return false;
	}

	set("alpha", 1.0f);
	LONGLONG duration;
	hr = _ms->GetStopPosition(&duration);
	if (SUCCEEDED(hr)) {
		_duration = duration / 10000;
		_log.information(Poco::format("duration: %d", _duration));
	}
	_current = 0;
	_mediaID = media->id();
	_finished = false;
	return true;
}

/**
 * 再生
 */
void DSContent::play() {
	if (_mc) {
		HRESULT hr = _mc->Run();
		_playing = true;
	}
}

/**
 * 停止
 */
void DSContent::stop() {
	if (_mc) {
		HRESULT hr = _mc->Stop();
	}
	_playing = false;
}

/**
 * 再生中かどうか
 */
const bool DSContent::playing() const {
	return _playing;
}

const bool DSContent::finished() {
	return _finished;
}

/** ファイルをクローズします */
void DSContent::close() {
	stop();
	SAFE_RELEASE(_me);
	SAFE_RELEASE(_ms);
	SAFE_RELEASE(_mc);
	SAFE_RELEASE(_vr);
	SAFE_RELEASE(_vmr9);
	SAFE_RELEASE(_gb);
	_mediaID.clear();
	//CoUninitialize();
}

void DSContent::process(const DWORD& frame) {
	if (_ms) {
		LONGLONG current;
		LONGLONG stop;
		_ms->GetPositions(&current, &stop);
		_current = current / 10000;
		//_duration = stop;
	}
	if (_me) {
		long eventCode;
		long param1;
		long param2;
		HRESULT hr = _me->GetEvent(&eventCode, (LONG_PTR*)&param1, (LONG_PTR*)&param2, 0);
		if (SUCCEEDED(hr)) {
			if (EC_COMPLETE == eventCode) {
//				hr = _mp->put_CurrentPosition(0);
				_finished = true;
			}
			hr = _me->FreeEventParams(eventCode, param1, param2);
		}
	}

	unsigned long cu = _current / 1000;
	unsigned long re = (_duration - _current) / 1000;
	string t1 = Poco::format("%02lu:%02lu:%02lu.%02d", cu / 3600, cu / 60, cu % 60, (_current % 1000) / 30);
	string t2 = Poco::format("%02lu:%02lu:%02lu.%02d", re / 3600, re / 60, re % 60, ((_duration - _current) % 1000) / 30);
	set("time", Poco::format("%s %s", t1, t2));
	set("time_current", t1);
	set("time_remain", t2);
}

void DSContent::draw(const DWORD& frame) {
	if (!_mediaID.empty() && _playing) {
		if (_vr) {
			LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
			float alpha = getF("alpha");
			int cw = config().splitSize.cx;
			int ch = config().splitSize.cy;
			DWORD col = ((DWORD)(0xff * alpha) << 24) | 0xffffff;
			switch (config().splitType) {
			case 1:
				{
					int sx = 0, sy = 0, dx = 0, dy = 0;
					int cww = 0;
					int chh = ch;
					while (dx < config().mainRect.right) {
						if ((sx + cw) >= _vr->width()) {
							// はみ出る
							cww = _vr->width() - sx;
							_vr->draw(dx, dy, cww, chh, 0, col, sx, sy, cww, chh);
							sx = 0;
							sy += ch;
							if (sy >= _vr->height()) break;
							if (_vr->height() - sy < ch) chh = _vr->height() - sy;
							// クイの分
							_vr->draw(dx + cww, dy, cw - cww, chh, 0, col, sx, sy, cw - cww, chh);
							sx += (cw - cww);
						} else {
							_vr->draw(dx, dy, cw, chh, 0, col, sx, sy, cw, chh);
							sx += cw;
						}
						// _log.information(Poco::format("split dst: %04d,%03d src: %04d,%03d", dx, dy, sx, sy));
						dy += ch;
						if (dy >= config().stageRect.bottom * config().splitCycles) {
							dx += cw;
							dy = 0;
						}
					}
				}
				break;

			case 2:
				{
					int sw = L(_w / cw);
					if (sw <= 0) sw = 1;
					int sh = L(_h / ch);
					if (sh <= 0) sh = 1;
					for (int sy = 0; sy < sh; sy++) {
						int ox = (sy % 2) * cw * 8 + config().stageRect.left;
						int oy = (sy / 2) * ch * 4 + config().stageRect.top;
						// int ox = (sy % 2) * cw * 8;
						// int oy = (sy / 2) * ch * 4;
						for (int sx = 0; sx < sw; sx++) {
							int dx = (sx / 4) * cw;
							int dy = ch * 3 - (sx % 4) * ch;
							_vr->draw(ox + dx, oy + dy, cw, ch, 0, col, sx * cw, sy * ch, cw, ch);
							// _renderer.drawTexture(ox + dx, oy + dy, cw, ch, sx * cw, sy * ch, cw, ch, _target, col, col, col, col);
						}
					}
				}
				break;

			case 11:
				{
					if (_vr->height() == 120) {
						_vr->draw(0, 360, 320, 120, 0, col, 2880, 0, 320, 120);
						_vr->draw(0, 240, 960, 120, 0, col, 1920, 0, 960, 120);
						_vr->draw(0, 120, 960, 120, 0, col,  960, 0, 960, 120);
						_vr->draw(0,   0, 960, 120, 0, col,    0, 0, 960, 120);
					} else {
						int w = config().mainRect.right;
						int h = config().mainRect.bottom;
						if (_vr->width() > w || _vr->height() > h) {
							device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
							device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
							_vr->draw( 0, 0, w, h, 1, col);
							device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
							device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
						} else {
							_vr->draw(0, 0, -1, -1, 0, col);
						}
					}
				}
				break;
			default:
				{
					RECT rect = config().stageRect;
					string aspectMode = get("aspect-mode");
					if (aspectMode == "fit") {
						device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
						device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
						_vr->draw(L(_x), L(_y), L(_w), L(_h), 0, col);
						device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
						device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);

					} else {
						device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
						device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
						if (alpha > 0.0f) {
							DWORD base = ((DWORD)(0xff * alpha) << 24) | 0x000000;
							_renderer.drawTexture(_x, _y, _w, _h, NULL, 0, base, base, base, base);
							float dar = _vr->getDisplayAspectRatio();
							if (_h * dar > _w) {
								// 画角よりディスプレイサイズは横長
								long h = _w / dar;
								long dy = (_h - h) / 2;
								_vr->draw(L(_x), L(_y + dy), L(_w), h, 0, col);
							} else {
								long w = _h * dar;
								long dx = (_w - w) / 2;
								_vr->draw(L(_x + dx), L(_y), w, L(_h), 0, col);
							}
						}

						device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
						device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
					}
				}
				break;
			}
		}
	}
}


/**
 * フィルタの指定した方向の接続されていないピンを探して返します。
 */
bool DSContent::getPin(IBaseFilter *pFilter, IPin** pPin, PIN_DIRECTION PinDir) {
    bool bFound = false;
    IEnumPins *pEnum = NULL;
    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (SUCCEEDED(hr)) {
	    while (!bFound && pEnum->Next(1, pPin, 0) == S_OK) {
	        PIN_DIRECTION PinDirThis;
	        (*pPin)->QueryDirection(&PinDirThis);
	        if (PinDir == PinDirThis) {
				IPin *pToPin = NULL;
				hr = (*pPin)->ConnectedTo(&pToPin);
				if (SUCCEEDED(hr)) {
					SAFE_RELEASE(pToPin);
					SAFE_RELEASE(*pPin);
				} else if (hr == VFW_E_NOT_CONNECTED) {
					bFound = true;
				}
			}
	    }
		// ピンの参照は残したまま戻す
		SAFE_RELEASE(pEnum);
    }
    return bFound;
}

/* 指定したフィルタの入力ピンを返します */
bool DSContent::getInPin(IBaseFilter *pFilter, IPin** pPin) {
    return getPin(pFilter, pPin, PINDIR_INPUT);
}

/* 指定したフィルタの出力ピンを返します */
bool DSContent::getOutPin(IBaseFilter *pFilter, IPin** pPin) {
    return getPin(pFilter, pPin, PINDIR_OUTPUT);
}

/**
 * グラフビルダのフィルタをダンプします。
 * (ビデオレンダラのような)ウィンドウフィルタがある場合は問題のある可能性があるので数を返します
 */
int DSContent::dumpFilter(IGraphBuilder* gb) {
	if (!gb) return 0;

	IEnumFilters *pEnum = NULL;
	HRESULT hr = gb->EnumFilters(&pEnum);

	int count = 0;
	int vcount = 0;
    IBaseFilter *pFilter = NULL;
    while (pEnum->Next(1, &pFilter, 0) == S_OK) {
    	FILTER_INFO info;
		HRESULT hr = pFilter->QueryFilterInfo(&info);
		if (SUCCEEDED(hr)) {
			wstring wname(info.achName);
			string name;
			Poco::UnicodeConverter::toUTF8(wname, name);
			_log.information(Poco::format("filter: %s", name));
			SAFE_RELEASE(info.pGraph);
		}
		IVideoWindow *vw;
        hr = pFilter->QueryInterface(&vw);
		if (SUCCEEDED(hr) && vw != NULL) {
			vcount++;
			SAFE_RELEASE(vw);
		}
		SAFE_RELEASE(pFilter);
 		count++;
    }
	SAFE_RELEASE(pEnum);
	_log.information(Poco::format("dump filters: %d(in windowed filters: %d)", count, vcount));
	return vcount;
}
