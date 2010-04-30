#include "CaptureScene.h"
#include <Poco/Util/XMLConfiguration.h>
#include <Poco/UnicodeConverter.h>

CaptureScene::CaptureScene(Renderer& renderer): Scene(renderer),
	_frame(0), _deviceNo(0), _routePinNo(0), _deviceW(640), _deviceH(480), _deviceFPS(30), _deviceVideoType(MEDIASUBTYPE_YUY2),
	_previewX(0), _previewY(0),_previewW(320), _previewH(240),
	_device(NULL), _gb(NULL), _capture(NULL), _vr(NULL), _mc(NULL), _cameraImage(NULL), _surface(NULL), _gray(NULL)
{
}

CaptureScene::~CaptureScene() {
	releaseFilter();
	SAFE_RELEASE(_cameraImage);
	SAFE_RELEASE(_surface);
	SAFE_DELETE(_gray);
	_log.information("*release capture-scene");
}

bool CaptureScene::initialize() {
	_log.information("*initialize CaptureScene");

	try {
		Poco::Util::XMLConfiguration* xml = new Poco::Util::XMLConfiguration("capture-config.xml");
		_useStageCapture = xml->getBool("device[@useStageCapture]", false);
		_deviceNo = xml->getInt("device[@no]", 0);
		_routePinNo = xml->getInt("device[@route]", 0);
		if (_useStageCapture) {
			_deviceW = xml->getInt("device[@w]", 640);
			_deviceH = xml->getInt("device[@h]", 480);
			//_deviceW = config().stageRect.right;
			//_deviceH = config().stageRect.bottom;
		} else {
			_deviceW = xml->getInt("device[@w]", 640);
			_deviceH = xml->getInt("device[@h]", 480);
		}
		_deviceFPS = xml->getInt("device[@fps]", 30);
		string type = Poco::toLower(xml->getString("device[@type]", "yuv2"));
		if (type == "rgb24") {
			_deviceVideoType = MEDIASUBTYPE_RGB24;
		} else if (type == "yuv2") {
			_deviceVideoType = MEDIASUBTYPE_YUY2;
		}

		_previewX = xml->getInt("preview.x", 0);
		_previewY = xml->getInt("preview.y", 0);
		_previewW = xml->getInt("preview.width", 320);
		_previewH = xml->getInt("preview.height", 240);

		xml->release();
	} catch (Poco::Exception& ex) {
		_log.warning(ex.displayText());
	}

	if (_useStageCapture || createFilter()) {
		_cameraImage = _renderer.createRenderTarget(_deviceW, _deviceH, D3DFMT_A8R8G8B8);
		_surface = _renderer.createLockableSurface(_deviceW, _deviceH, D3DFMT_A8R8G8B8);
		_gray = new BYTE[_deviceW * _deviceH];
		_log.information(Poco::format("camera image: %dx%d %s-mode %s", _deviceW, _deviceH, string(_useStageCapture?"stage":"capture"), string(_cameraImage?"OK":"NG")));
		return true;
	}
	return false;
}

bool CaptureScene::createFilter() {
	HRESULT hr;
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&_gb);
	if (FAILED(hr)) {
		_log.warning(Poco::format("failed create filter graph: %s", errorText(hr)));
		return false;
	}
	_vr = new DSVideoRenderer(_renderer, NULL, &hr);
	if (FAILED(hr = _gb->AddFilter(_vr, L"DSVideoRenderer"))) {
		_log.warning(Poco::format("failed add filter: %s", errorText(hr)));
		return false;
	}
	hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void**)&_capture);
	if (FAILED(hr)) {
		_log.warning(Poco::format("failed create capture graph builder2: %s", errorText(hr)));
		return false;
	}
	hr = _capture->SetFiltergraph(_gb);
	if (FAILED(hr)) {
		_log.warning(Poco::format("setup failed capture graph builder2: %s", errorText(hr)));
		return false;
	}

	string deviceName;
	IBaseFilter* src;
	if (fetchDevice(CLSID_VideoInputDeviceCategory, _deviceNo, &src, deviceName)) {
		hr = _gb->AddFilter(src, L"Video Capture Source");
		if (FAILED(hr)) {
			_log.warning("failed add capture source");
		} else {
			SAFE_RELEASE(_device);
			_device = src;
			routeCrossbar(src, _routePinNo);
		}

		IPin* renderPin = NULL;
		hr = _capture->FindPin(src, PINDIR_OUTPUT, &PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, FALSE, 0, &renderPin);
		if (FAILED(hr)) {
			_log.warning("*not found preview pin.");
			hr = _capture->FindPin(src, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, FALSE, 0, &renderPin);
			if (FAILED(hr)) {
				_log.warning("*not found capture pin.");
				return false;
			} else {
				_log.information("rendered capture pin");
			}
		} else {
			_log.information("rendered preview pin");
		}
		PIN_INFO info;
		renderPin->QueryPinInfo(&info);
		info.pFilter->Release();
		wstring wname(info.achName);
		string name;
		Poco::UnicodeConverter::toUTF8(wname, name);
		_log.information(Poco::format("render pin: %s", name));

		// 出力サイズの設定
		IAMStreamConfig* sc;
		hr = renderPin->QueryInterface(&sc);
		if (SUCCEEDED(hr)) {
			int count;
			int size;
			hr = sc->GetNumberOfCapabilities(&count, &size);
			float maxFPS = 0;
			long maxW = 0;
			long maxH = 0;
			int fixedCount = 0;
			int bestConfig = -1;
			int hitConfig = -1;
			VIDEO_STREAM_CONFIG_CAPS scc;
			for (int i = 0; i < count; i++) {
				AM_MEDIA_TYPE* amt;
				hr = sc->GetStreamCaps(i, &amt, reinterpret_cast<BYTE*>(&scc));
				if (SUCCEEDED(hr)) {
					float fps = 10000000.0f / scc.MinFrameInterval;
					string videoType;
					if (IsEqualGUID(amt->subtype, MEDIASUBTYPE_RGB24)) {
						videoType = "RGB24";
					} else if (IsEqualGUID(amt->subtype, MEDIASUBTYPE_RGB32)) {
						videoType = "RGB32";
					} else if (IsEqualGUID(amt->subtype, MEDIASUBTYPE_RGB555)) {
						videoType = "RGB555";
					} else if (IsEqualGUID(amt->subtype, MEDIASUBTYPE_RGB565)) {
						videoType = "RGB565";
					} else if (IsEqualGUID(amt->subtype, MEDIASUBTYPE_YVYU)) {
						videoType = "YVYU";
					} else if (IsEqualGUID(amt->subtype, MEDIASUBTYPE_UYVY)) {
						videoType = "UYVY";
					} else if (IsEqualGUID(amt->subtype, MEDIASUBTYPE_IYUV)) {
						videoType = "IYUV";
					} else if (IsEqualGUID(amt->subtype, MEDIASUBTYPE_YUY2)) {
						videoType = "YUY2";
					} else if (IsEqualGUID(amt->subtype, MEDIASUBTYPE_Y41P)) {
						videoType = "Y41P";
					} else if (IsEqualGUID(amt->subtype, MEDIASUBTYPE_YVU9)) {
						videoType = "YVU9";
					} else if (IsEqualGUID(amt->subtype, MEDIASUBTYPE_YV12)) {
						videoType = "YV12";
					} else if (IsEqualGUID(amt->subtype, MEDIASUBTYPE_YV12)) {
						videoType = "YV12";
					} else if (IsEqualGUID(amt->subtype, MEDIASUBTYPE_MJPG)) {
						videoType = "MJPG";
					} else {
						videoType = Poco::format("unknown 0x%lx", amt->subtype.Data1);
					}
					bool outputFixed = scc.MinOutputSize.cx == scc.MaxOutputSize.cx && scc.MinOutputSize.cy == scc.MaxOutputSize.cy;
					long mw = scc.MinOutputSize.cx;
					long mh = scc.MinOutputSize.cy;
					long gx = scc.OutputGranularityX;
					long gy = scc.OutputGranularityY;
				    long w = scc.MaxCroppingSize.cx;
				    long h = scc.MaxCroppingSize.cy;
					ULONG videoStandard = scc.VideoStandard;
					string vinfo = Poco::format("min:%ldx%ld max crop:%ldx%ld fps:%0.2hf", mw, mh, w, h, fps);
					_log.information(Poco::format("capability(%02d) video:%s %lu size fixed:%s %s", i, videoType, videoStandard, string(outputFixed?"true":"false"), vinfo));

					if (IsEqualGUID(amt->formattype, FORMAT_VideoInfo) && IsEqualGUID(amt->subtype, _deviceVideoType)) {
						// ビデオタイプが一致
						if (!outputFixed && mw < _deviceW && mh < _deviceH && (maxW < w || maxH < h)) {
							maxW = w;
							maxH = h;
							bestConfig = i;
							_log.information(Poco::format("best configuration: %d", i));
						}
						if (w == _deviceW && h == _deviceH) {
							hitConfig = i;
							_log.information(Poco::format("hit configuration: %d", i));
						}
					}
					if (outputFixed) fixedCount++;
					DeleteMediaType(amt);
				}
			}

			if (fixedCount == count) {
				// 固定選択式
				if (hitConfig >= 0) {
					_log.information(Poco::format("fixed configuration setting: %d", hitConfig));
					AM_MEDIA_TYPE* amt;
					hr = sc->GetStreamCaps(hitConfig, &amt, reinterpret_cast<BYTE*>(&scc));
					if SUCCEEDED(hr) {
						long w = scc.MaxCroppingSize.cx;
						long h = scc.MaxCroppingSize.cy;
						float fps = 10000000.0f / scc.MinFrameInterval;
						VIDEOINFOHEADER* info = reinterpret_cast<VIDEOINFOHEADER*>(amt->pbFormat);
						info->bmiHeader.biWidth = w;
						info->bmiHeader.biHeight = h;
						// info->AvgTimePerFrame = 10000000 / _deviceFPS;
						// info->AvgTimePerFrame = scc.MinFrameInterval;
						hr = sc->SetFormat(amt);
						if (FAILED(hr)) {
							_log.warning(Poco::format("failed set format: %s", errorText(hr)));
						} else {
							_log.information(Poco::format("set size:%ldx%ld fps:%0.2hf", w, h, fps));
						}
						DeleteMediaType(amt);
					}
				} else {
					_log.warning(Poco::format("not found fixed configuration setting: %dx%d", _deviceW, _deviceH));
				}

			} else {
				// ベスト設定をベースに使う
				_log.information(Poco::format("best configuration based setting: %d", bestConfig));
				AM_MEDIA_TYPE* amt;
				hr = sc->GetStreamCaps(bestConfig, &amt, reinterpret_cast<BYTE*>(&scc));
				if (SUCCEEDED(hr)) {
					VIDEOINFOHEADER* info = reinterpret_cast<VIDEOINFOHEADER*>(amt->pbFormat);
					info->bmiHeader.biWidth = _deviceW;
					info->bmiHeader.biHeight = _deviceH;
					// info->AvgTimePerFrame = 10000000 / _deviceFPS;
					hr = sc->SetFormat(amt);
					if (FAILED(hr)) {
						_log.warning(Poco::format("failed set format: %s", errorText(hr)));
					} else {
						_log.information(Poco::format("set size:%dx%d fps:%d", _deviceW, _deviceH, _deviceFPS));
					}
					DeleteMediaType(amt);
				}
			}
//			SAFE_RELEASE(sc);
		}

//		hr = _capture->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, src, NULL, NULL);
		hr = _gb->Render(renderPin);
		SAFE_RELEASE(renderPin);
//		SAFE_RELEASE(src);
		if (dumpFilter(_gb) == 0) {
			// 正しくレンダリングできた
//			IMediaSeeking* ms;
//			IMediaEvent* me;
			IMediaFilter* mf;
			hr = _gb->QueryInterface(&mf);
			if (SUCCEEDED(hr)) {
				// リアルタイムでキャプチャするために基準タイマを外す
				hr = mf->SetSyncSource(NULL);
				SAFE_RELEASE(mf);
			}
			hr = _gb->QueryInterface(&_mc);
			if (SUCCEEDED(hr)) {
				hr = _mc->Run();
			} else {
				_log.warning("failed query interface: IMediaControl");
			}
		}
		return true;

	} else {
		_log.warning("not found video input device");
	}
	return false;
}

void CaptureScene::releaseFilter() {
	if (_mc) {
		HRESULT hr = _mc->Stop();
		if (SUCCEEDED(hr)) {
			// DirectShow停止待ち
			for (;;) {
				OAFilterState fs;
				hr = _mc->GetState(300, &fs);
				if (hr == State_Stopped) {
					break;
				}
				Sleep(100);
			}
		}
	}

	SAFE_RELEASE(_mc);
	SAFE_RELEASE(_vr);
	SAFE_RELEASE(_capture);
	SAFE_RELEASE(_device);
	SAFE_RELEASE(_gb);
}

LPDIRECT3DTEXTURE9 CaptureScene::getCameraImage() {
	return _cameraImage;
}

LPBYTE CaptureScene::getGrayScale() {
	return _gray;
}

void CaptureScene::process() {
	if (_cameraImage && _renderer.getRenderTargetData(_cameraImage, _surface)) {
		D3DLOCKED_RECT lockedRect = {0};
		if (SUCCEEDED(_surface->LockRect(&lockedRect, NULL, 0))) {
			LPBYTE src = (LPBYTE)lockedRect.pBits;
			long len = _deviceW * _deviceH * 4;
			long pos = 0;
			for (long i = 0; i < len; i += 4) {
				_gray[pos++] = src[i + 1];
				//int y = 0.299f * src[i + 2] + 0.587f * src[i + 1] + 0.114f * src[i + 0];
				//if (y > 255) y = 255;
				//_gray[pos++] = y;
			}
			_surface->UnlockRect();
		}
	}
	_frame++;
}

void CaptureScene::draw1() {
	if (_cameraImage) {
		DWORD col = 0xffffffff;
		D3DSURFACE_DESC desc;
		HRESULT hr = _cameraImage->GetLevelDesc(0, &desc);
		VERTEX dst[] =
		{
			{F(0              - 0.5), F(0               - 0.5), 0.0f, 1.0f, col, 0, 0},
			{F(0 + desc.Width - 0.5), F(0               - 0.5), 0.0f, 1.0f, col, 1, 0},
			{F(0              - 0.5), F(0 + desc.Height - 0.5), 0.0f, 1.0f, col, 0, 1},
			{F(0 + desc.Width - 0.5), F(0 + desc.Height - 0.5), 0.0f, 1.0f, col, 1, 1}
		};

		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		if (_vr) {
			LPDIRECT3DSURFACE9 orgRT;
			hr = device->GetRenderTarget(0, &orgRT);

			LPDIRECT3DSURFACE9 surface;
			_cameraImage->GetSurfaceLevel(0, &surface);
			hr = device->SetRenderTarget(0, surface);
			device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
			DWORD col = 0xffffffff;
			_renderer.drawTexture(0, 0, desc.Width, desc.Height, _vr->getTexture(), 1, col, col, col, col);
			SAFE_RELEASE(surface);

			hr = device->SetRenderTarget(0, orgRT);
			SAFE_RELEASE(orgRT);
		} else if (_useStageCapture) {
			LPDIRECT3DSURFACE9 src,dst;
			LPDIRECT3DTEXTURE9 capture = _renderer.getCaptureTexture();
			capture->GetSurfaceLevel(0, &src);
			_cameraImage->GetSurfaceLevel(0, &dst);
			hr = device->StretchRect(src, NULL, dst, NULL, D3DTEXF_POINT);
			if FAILED(hr) _log.warning("failed copy texture");
			SAFE_RELEASE(src);
			SAFE_RELEASE(dst);
		}
	}
}

void CaptureScene::draw2() {
	if (_visible) {
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		if (_vr) {
			device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			DWORD col = 0xffffffff;
			_renderer.drawTexture(_previewX, _previewY, _previewW, _previewH, _cameraImage, 0, col, col, col, col);
			device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			string s = Poco::format("LIVE! read %03lums", _vr->readTime());
			_renderer.drawFontTextureText(_previewX, _previewY, 8, 12, 0xccff6666, s);
		} else if (_useStageCapture) {
			device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			DWORD col = 0xffffffff;
			_renderer.drawTexture(_previewX, _previewY, _previewW, _previewH, _cameraImage, 0, col, col, col, col);
			device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			_renderer.drawFontTextureText(_previewX, _previewY, 8, 12, 0xccff6666, "STAGE RECORDING");
		} else {
			_renderer.drawFontTextureText(_previewX, _previewY, 8, 12, 0xccff6666, "NO SIGNAL");
		}
	}
}


/* 指定したデバイスクラスのフィルタを取得します */
bool CaptureScene::fetchDevice(REFCLSID clsidDeviceClass, int index, IBaseFilter** pBf, string& deviceName) {
	ICreateDevEnum* pDevEnum =NULL;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pDevEnum);
	if (FAILED(hr)) {
		_log.warning(Poco::format("system device enumeration failed: %s", errorText(hr)));
		return false;
	}

	IEnumMoniker* pEnumCat = NULL;
	hr = pDevEnum->CreateClassEnumerator(clsidDeviceClass, &pEnumCat, NULL);
	pDevEnum->Release();
	if (hr != S_OK) {
		_log.warning("category enumeration zero count or failed.");
		return false;
	}

	IMoniker* moniker = NULL;
	int count = 0;
	bool lookup = false;
	while (SUCCEEDED(pEnumCat->Next(1, &moniker, NULL)) && count <= index) {
		if (count == index) {
			IPropertyBag* propBag;
	        hr = moniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&propBag);
			if (SUCCEEDED(hr)) {
		        // フィルタのフレンドリ名を取得するには、次の処理を行う。
				VARIANT varName;
				VariantInit(&varName);
				hr = propBag->Read(L"FriendlyName", &varName, 0);
				if (SUCCEEDED(hr)) {
					UINT nLength = SysStringLen(V_BSTR(&varName) != NULL?V_BSTR(&varName):OLESTR(""));
					if (nLength > 0) {
						vector<WCHAR> wname(nLength);
						wcsncpy(&wname[0], V_BSTR(&varName), nLength);
						wname.push_back('\0');
						Poco::UnicodeConverter::toUTF8(wstring(&wname[0]), deviceName);
						_log.information(Poco::format("fetch device: %s", deviceName));
					}
				}
				VariantClear(&varName);	
				propBag->Release();
	        }
			hr = moniker->BindToObject(0, 0, IID_IBaseFilter, (void**)pBf);
			if (SUCCEEDED(hr)) {
				lookup = true;
			}
		}
		moniker->Release();
		count++;
	}
	pEnumCat->Release();
	_log.information(Poco::format("device result: %s", string(lookup?"LOOKUP":"NOT FOUND")));
	return lookup;
}

/* クロスバーをルーティングします */
bool CaptureScene::routeCrossbar(IBaseFilter* src, int no) {
	IAMCrossbar* crossbar = NULL;
	HRESULT hr = _capture->FindInterface(&LOOK_UPSTREAM_ONLY, NULL, src, IID_IAMCrossbar, (void**)&crossbar);
	if (FAILED(hr)) {
		_log.warning(Poco::format("not found crossbar: %s", errorText(hr)));
		return false;
	}

	long cOutput = -1;
	long cInput = -1;
	hr = crossbar->get_PinCounts(&cOutput, &cInput);
	for (long i = 0; i < cOutput; i++) {
		long lRelated = -1;
		long lType = -1;
		long lRouted = -1;
		hr = crossbar->get_CrossbarPinInfo(FALSE, i, &lRelated, &lType);
		if (lType == PhysConn_Video_VideoDecoder) {
			hr = crossbar->get_IsRoutedTo(i, &lRouted);
			hr = crossbar->CanRoute(i, no);
			if (SUCCEEDED(hr)) {
				hr = crossbar->Route(i, no);
				crossbar->get_CrossbarPinInfo(TRUE, no, &lRelated, &lType);
				if (SUCCEEDED(hr)) {
					_log.information(Poco::format("crossbar routed to <%s>", getPinName(lType)));
					break;
				} else {
					_log.warning(Poco::format("crossbar can't routing to <%s>: %s", getPinName(lType), errorText(hr)));
				}
			}
		}
	}
	SAFE_RELEASE(crossbar);
	return true;
}

// フィルタ一覧
int CaptureScene::dumpFilter(IGraphBuilder* gb) {
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

// タイプと名前を関連付けるヘルパー関数。
const string CaptureScene::getPinName(long lType) {
	switch (lType) {
		case PhysConn_Video_Tuner:            return string("Video Tuner");
		case PhysConn_Video_Composite:        return string("Video Composite");
		case PhysConn_Video_SVideo:           return string("S-Video");
		case PhysConn_Video_RGB:              return string("Video RGB");
		case PhysConn_Video_YRYBY:            return string("Video YRYBY");
		case PhysConn_Video_SerialDigital:    return string("Video Serial Digital");
		case PhysConn_Video_ParallelDigital:  return string("Video Parallel Digital"); 
		case PhysConn_Video_SCSI:             return string("Video SCSI");
		case PhysConn_Video_AUX:              return string("Video AUX");
		case PhysConn_Video_1394:             return string("Video 1394");
		case PhysConn_Video_USB:              return string("Video USB");
		case PhysConn_Video_VideoDecoder:     return string("Video Decoder");
		case PhysConn_Video_VideoEncoder:     return string("Video Encoder");
	        
		case PhysConn_Audio_Tuner:            return string("Audio Tuner");
		case PhysConn_Audio_Line:             return string("Audio Line");
		case PhysConn_Audio_Mic:              return string("Audio Microphone");
		case PhysConn_Audio_AESDigital:       return string("Audio AES/EBU Digital");
		case PhysConn_Audio_SPDIFDigital:     return string("Audio S/PDIF");
		case PhysConn_Audio_SCSI:             return string("Audio SCSI");
		case PhysConn_Audio_AUX:              return string("Audio AUX");
		case PhysConn_Audio_1394:             return string("Audio 1394");
		case PhysConn_Audio_USB:              return string("Audio USB");
		case PhysConn_Audio_AudioDecoder:     return string("Audio Decoder");
	}
	return string("Unknown Type");
}

// エラー文字列を返します。
const string CaptureScene::errorText(HRESULT hr) {
	vector<WCHAR> err(1024);
	DWORD res = AMGetErrorText(hr, &err[0], 1024);
	string utf8;
	Poco::UnicodeConverter::toUTF8(wstring(&err[0]), utf8);
	string::size_type i = utf8.find("\n");
	if (string::npos != i) utf8 = utf8.substr(0, i); // エラー文字列から改行を除去
	return utf8;
}
