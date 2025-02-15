#include "CaptureScene.h"
#include <algorithm>
#include <Poco/Util/XMLConfiguration.h>
#include <Poco/UnicodeConverter.h>
#include "Utils.h"


CaptureScene::CaptureScene(Renderer& renderer): Scene(renderer),
	activeChangePlaylist(this, &CaptureScene::changePlaylist),
	_frame(0), _startup(0), _useStageCapture(false),
	_deviceNo(0), _routePinNo(0), _deviceW(640), _deviceH(480), _deviceFPS(30),
	_autoWhiteBalance(true), _whiteBalance(-100), _autoExposure(true), _exposure(-100),
	_flipMode(3), _deviceVideoType(MEDIASUBTYPE_YUY2),
	_px(0), _py(0),_pw(320), _ph(240), _spx(320), _spy(0),_spw(320), _sph(240),
	_device(NULL), _gb(NULL), _capture(NULL), _vr(NULL), _mc(NULL), _cameraImage(NULL),
	_sample(NULL), _surface(NULL), _fx(NULL), _data1(NULL), _data2(NULL), _data3(NULL),
	_lookup(NULL), _block(NULL), _activeBlock(NULL),
	_forceUpdate(false), _detectCount(0), _ignoreDetectCount(0), _main(NULL)
{
}

CaptureScene::~CaptureScene() {
	releaseFilter();
	SAFE_RELEASE(_cameraImage);
	SAFE_RELEASE(_sample);
	SAFE_RELEASE(_surface);
	SAFE_RELEASE(_fx);
	SAFE_DELETE(_data1);
	SAFE_DELETE(_data2);
	SAFE_DELETE(_data3);
	SAFE_DELETE(_lookup);
	SAFE_DELETE(_block);
	SAFE_DELETE(_activeBlock);
	_log.information("*release capture-scene");
}

bool CaptureScene::initialize() {
	_log.information("*initialize CaptureScene");

	bool useSampling = false;
	vector<int> activeBlocks;
	SetRect(&_clip, 0, 0, _deviceW, _deviceH);
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
		_useDeinterlace = xml->getBool("device[@deinterlace]", true);
		_flipMode = xml->getInt("device[@flipMode]", 3);
		string type = Poco::toLower(xml->getString("device[@type]", "yuv2"));
		if (type == "rgb24") {
			_deviceVideoType = MEDIASUBTYPE_RGB24;
		} else if (type == "rgb32") {
			_deviceVideoType = MEDIASUBTYPE_RGB32;
		} else if (type == "yuv2") {
			_deviceVideoType = MEDIASUBTYPE_YUY2;
		}
		int cx = xml->getInt("device[@cx]", 0);
		int cy = xml->getInt("device[@cy]", 0);
		int cw = xml->getInt("device[@cw]", _deviceW);
		int ch = xml->getInt("device[@ch]", _deviceH);
		SetRect(&_clip, cx, cy, cw, ch);
		string wb = Poco::toLower(xml->getString("device[@whitebalance]", "auto"));
		if (wb == "auto") {
			_autoWhiteBalance = true;
		} else {
			_autoWhiteBalance = false;
			_whiteBalance = -100;
			 Poco::NumberParser::tryParse(wb, _whiteBalance);
		}
		string exposure = Poco::toLower(xml->getString("device[@exposure]", "auto"));
		if (exposure == "auto") {
			_autoExposure = true;
		} else {
			_autoExposure = false;
			_exposure = -100;
			Poco::NumberParser::tryParse(exposure, _exposure);
		}

		_px = xml->getInt("preview.x", 0);
		_py = xml->getInt("preview.y", 0);
		_pw = xml->getInt("preview.width", 320);
		_ph = xml->getInt("preview.height", 240);

		useSampling = xml->hasProperty("sampling");
		if (useSampling) {
			_sw = xml->getInt("sampling.width", 0);
			_sh = xml->getInt("sampling.height", 0);
			_spx = xml->getInt("sampling.preview.x", _px + _pw);
			_spy = xml->getInt("sampling.preview.y", _py);
			_spw = xml->getInt("sampling.preview.width", _pw);
			_sph = xml->getInt("sampling.preview.height", _ph);
			_intervalsBackground = xml->getInt("sampling.intervalsBackground", 3600);
			_intervalsForeground = xml->getInt("sampling.intervalsForeground", 30);
			_blockThreshold = xml->getInt("sampling.blockThreshold", 20);
			_lookupThreshold = xml->getInt("sampling.lookupThreshold", 40);
			_detectThreshold = xml->getInt("sampling.detectThreshold", 100);
			_detectedPlaylist = xml->getString("sampling.detectedPlaylist", "");
			string activePlaylist = xml->getString("sampling.activePlaylist", "");
			if (!activePlaylist.empty()) svvitch::split(activePlaylist, ',', _activePlaylist);
			_ignoreDetectTime = xml->getInt("sampling.ignoreDetectTime", 15 * 60);
			string ab = xml->getString("sampling.activeBlocks", "");
			svvitch::parseMultiNumbers(ab, 0, _sw * _sh - 1, activeBlocks);
		}

		xml->release();
	} catch (Poco::Exception& ex) {
		_log.warning(ex.displayText());
	}

	if (_useStageCapture || createFilter()) {
		_cameraImage = _renderer.createRenderTarget(_clip.right, _clip.bottom, D3DFMT_A8R8G8B8);
		string s = Poco::format("clip:%ld,%ld,%ld,%ld", _clip.left, _clip.top, _clip.right, _clip.bottom);
		_log.information(Poco::format("camera image: %dx%d(%s) %s-mode %s", _deviceW, _deviceH, s, string(_useStageCapture?"stage":"capture"), string(_cameraImage?"OK":"NG")));

		if (useSampling) {
			_sample = _renderer.createRenderTarget(_sw, _sh, D3DFMT_A8R8G8B8);
			_renderer.colorFill(_sample, 0xff000000);
			_surface = _renderer.createLockableSurface(_sw, _sh, D3DFMT_A8R8G8B8);
			const long size = _sw * _sh;
			_data1 = new INT[size];
			_data2 = new INT[size];
			_data3 = new INT[size];
			_lookup = new BOOL[size];
			_block = new INT[size];
			_activeBlock = new BOOL[size];
			for (long i = 0; i < size; i++) {
				_data1[i] = -1;
				_data2[i] = -1;
				_data3[i] = -1;
				_lookup[i] = false;
				_block[i] = 0;
				_activeBlock[i] = activeBlocks.empty() || std::find(activeBlocks.begin(), activeBlocks.end(), i) != activeBlocks.end();
			}
			_fx = _renderer.createEffect("fx/rgb2hsv.fx");
			if (_fx) {
				HRESULT hr = _fx->SetTechnique("convertTechnique");
				if FAILED(hr) _log.warning("failed effect not set technique convertTechnique");
				hr = _fx->SetTexture("frame1", _cameraImage);
				if FAILED(hr) _log.warning("failed effect not set texture1");
				hr = _fx->SetTexture("frame2", _sample);
				if FAILED(hr) _log.warning("failed effect not set texture2");
			} else {
				return false;
			}
			_log.information(Poco::format("sampling image: %dx%d", _sw, _sh));
		}
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
	_vr = new DSVideoRenderer(_renderer, true, NULL, &hr);
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

	IBaseFilter* src = NULL;
	if (fetchDevice(CLSID_VideoInputDeviceCategory, _deviceNo, &src)) {
		hr = _gb->AddFilter(src, L"Video Capture Source");
		if (FAILED(hr)) {
			_log.warning("failed add capture source");
		} else {
			SAFE_RELEASE(_device);
			_device = src;
			routeCrossbar(src, _routePinNo);
			setWhiteBalance(src, _autoWhiteBalance, _whiteBalance);
			setExposure(src, _autoExposure, _exposure);
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
			// SAFE_RELEASE(sc);
		}

		// hr = _capture->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, src, NULL, NULL);

		// add deinterlacer
		if (_deviceH > 240 && _useDeinterlace) {
			IBaseFilter* deinterlacer = NULL;
			if (fetchDevice(CLSID_LegacyAmFilterCategory, 0, &deinterlacer, string("honestech Deinterlacer"))) {
				hr = _gb->AddFilter(deinterlacer, L"Deinterlacer");
				if (SUCCEEDED(hr)) {
					IPin* inPin = NULL;
					if (getInPin(deinterlacer, &inPin)) {
						hr = _gb->Connect(renderPin, inPin);
						inPin->Release();
						if (SUCCEEDED(hr)) {
							SAFE_RELEASE(renderPin);
							if (!getOutPin(deinterlacer, &renderPin)) {
								_log.warning(Poco::format("failed not get deinterlacer output-pin: %s", errorText(hr)));
							} else {
								_log.information("add&connect deinterlacer");
							}
						} else {
							_log.warning(Poco::format("failed not connect deinterlacer input-pin: %s", errorText(hr)));
						}
					} else {
						_log.warning(Poco::format("failed not connect deinterlacer input-pin: %s", errorText(hr)));
					}
				} else {
					_log.warning("failed not add deinterlacer");
				}
			} else {
				_log.warning("not found deinterlacer");
			}
		}

		hr = _gb->Render(renderPin);
		SAFE_RELEASE(renderPin);
		// SAFE_RELEASE(src);
		if (dumpFilter(_gb) == 0) {
			// 正しくレンダリングできた
			// IMediaSeeking* ms;
			// IMediaEvent* me;
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

void CaptureScene::process() {
	if (!_main) {
		_main = dynamic_cast<MainScenePtr>(_renderer.getScene("main"));
	}
	if (_startup < 100) {
		_startup++;
		return;
	}

	if (_sample) {
		if (_renderer.getRenderTargetData(_sample, _surface)) {
			D3DLOCKED_RECT lockedRect = {0};
			if (SUCCEEDED(_surface->LockRect(&lockedRect, NULL, 0))) {
				LPBYTE src = (LPBYTE)lockedRect.pBits;
				const long size = _sw * _sh * 4;
				long pos = 0;
				for (long i = 0; i < size; i += 4) {
					_data1[pos++] = src[i + 2];
				}
				_surface->UnlockRect();
			}
		}

		const long size = _sw * _sh;
		int lookup = 0;
		for (long i = 0; i < size; i++) {
			 _lookup[i] = false;
			int d1 = 255 - _data2[i] + _data3[i];
			int d2 = abs(_data2[i] - _data3[i]);
			if (d1 > d2) {
				d1 = d2;
			}
			_block[i] = d1;
			if (_activeBlock[i] && d1 > _blockThreshold) {
				_lookup[i] = true;
				lookup++;
			}
		}
		const int lookupThreshold = _sw * _sh * _lookupThreshold / 100;
		if (_ignoreDetectCount > 0) {
			_ignoreDetectCount--;
			if (_detectCount > 0) _detectCount--;
		} else {
			if (lookup >= lookupThreshold) {
				_detectCount++;
			} else {
				if (_detectCount > 0) _detectCount--;
			}
			if (_detectCount > _detectThreshold) {
				if (!_detectedPlaylist.empty() && _main) {
					string current = _main->getStatus("current-playlist-id");
					if (_activePlaylist.empty() || std::find(_activePlaylist.begin(), _activePlaylist.end(), current) != _activePlaylist.end()) {
						if (current != _detectedPlaylist) {
							activeChangePlaylist();
						} else {
							_log.information(Poco::format("already playing: %s", _detectedPlaylist));
						}
					}
				}
				//_detectCount = 0;
				_ignoreDetectCount = _ignoreDetectTime;
			}
		}
		// 背景のサンプリング
		if (_frame % _intervalsBackground == 0 || _forceUpdate) {
			if (_detectCount == 0 && _ignoreDetectCount == 0) {
				if (_data2[0] < 0) {
					for (long i = 0; i < size; i++) {
						_data2[i] = _data1[i];
					}
				} else {
					for (long i = 0; i < size; i++) {
						_data2[i] = (_data1[i] + _data2[i]) >> 1;
					}
				}
				_forceUpdate = false;
			} else {
				_forceUpdate = true;
			}
		}
		// 動体のサンプリング
		if (_frame % _intervalsForeground == 0) {
			if (_data3[0] < 0) {
				for (long i = 0; i < size; i++) {
					_data3[i] = _data1[i];
				}
			} else {
				for (long i = 0; i < size; i++) {
					_data3[i] = (_data1[i] + _data3[i]) >> 1;
				}
			}
		}
		_frame++;

	} else {
		// _log.warning("failed get render target data");
	}
}

void CaptureScene::draw1() {
	if (_cameraImage) {
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		if (_vr) {
			LPDIRECT3DSURFACE9 orgRT = NULL;
			HRESULT hr = device->GetRenderTarget(0, &orgRT);

			LPDIRECT3DSURFACE9 surface;
			_cameraImage->GetSurfaceLevel(0, &surface);
			hr = device->SetRenderTarget(0, surface);
			DWORD col = 0xffffffff;
			_vr->draw(-_clip.left, -_clip.top, _deviceW, _deviceH, 0, _flipMode, col, col, col, col);
			SAFE_RELEASE(surface);

			if (_sample) {
				D3DSURFACE_DESC desc;
				HRESULT hr = _sample->GetLevelDesc(0, &desc);
				VERTEX dst[] =
				{
					{F(0              - 0.5), F(0               - 0.5), 0.0f, 1.0f, col, 0, 0},
					{F(0 + desc.Width - 0.5), F(0               - 0.5), 0.0f, 1.0f, col, 1, 0},
					{F(0              - 0.5), F(0 + desc.Height - 0.5), 0.0f, 1.0f, col, 0, 1},
					{F(0 + desc.Width - 0.5), F(0 + desc.Height - 0.5), 0.0f, 1.0f, col, 1, 1}
				};
				_sample->GetSurfaceLevel(0, &surface);
				hr = device->SetRenderTarget(0, surface);
				_fx->Begin(NULL, 0);
				_fx->BeginPass(0);
				device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, VERTEX_SIZE);
				_fx->EndPass();
				_fx->End();
				SAFE_RELEASE(surface);
			}

			hr = device->SetRenderTarget(0, orgRT);
			SAFE_RELEASE(orgRT);
		} else if (_useStageCapture) {
			_renderer.copyTexture(_renderer.getCaptureTexture(), _cameraImage);
		} else {
		}
	}
}

void CaptureScene::draw2() {
	if (config().viewStatus) {
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		if (_vr) {
			device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			DWORD col = 0xffffffff;
			_renderer.drawTexture(_px, _py, _pw, _ph, _cameraImage, 0, col, col, col, col);
			device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			if (_sample) {
				col = 0xccffffff;
				_renderer.drawTexture(_spx, _spy, _spw, _sph, _sample, 0, col, col, col, col);
				const int sw = _spw / _sw;
				const int sh = _sph / _sh;
				for (int y = 0; y < _sh; y++) {
					for (int x = 0; x < _sw; x++) {
						const int i = y * _sw + x;
						if (_activeBlock[i]) {
							if (_lookup[i]) {
								_renderer.drawFontTextureText(_spx + x * sw, _spy + y * sh, sw, sh, 0xccff3333, "*");
							} else {
								_renderer.drawFontTextureText(_spx + x * sw, _spy + y * sh, sw, sh, 0xccffffff, "*");
							}
						}
						const BYTE data1 = _data1[i];
						const BYTE data2 = _data2[i];
						const BYTE data3 = _data3[i];
						const BYTE block = _block[i];
						const int px = _spw + x * 22;
						_renderer.drawFontTextureText(_spx + px                     , _spy + y * 10, 10, 10, 0xccff3333, Poco::format("%2?X", data1));
						_renderer.drawFontTextureText(_spx + px + (5 + _sw * 22)    , _spy + y * 10, 10, 10, 0xccff3333, Poco::format("%2?X", data2));
						_renderer.drawFontTextureText(_spx + px + (5 + _sw * 22) * 2, _spy + y * 10, 10, 10, 0xccff3333, Poco::format("%2?X", data3));
						_renderer.drawFontTextureText(_spx + px + (5 + _sw * 22) * 3, _spy + y * 10, 10, 10, 0xccff3333, Poco::format("%2?X", block));
					}
				}
				_renderer.drawFontTextureText(_spx + _spw, _spy + _sh * 10, 10, 10, 0xcc33ccff, Poco::format("detect:%3d ignore:%3d", _detectCount, _ignoreDetectCount));
				if (_forceUpdate) {
					_renderer.drawFontTextureText(_spx + _spw, _spy + _sh * 10 + 10, 10, 10, 0xccffcc00, "*");
				}
			}
			string s = Poco::format("LIVE! read %03lums", _vr->readTime());
			_renderer.drawFontTextureText(_px, _py, 10, 10, 0xccff3333, s);
		} else if (_useStageCapture) {
			device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			DWORD col = 0xffffffff;
			_renderer.drawTexture(_px, _py, _pw, _ph, _cameraImage, 0, col, col, col, col);
			device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			_renderer.drawFontTextureText(_px, _py, 10, 10, 0xccff3333, "STAGE RECORDING");
		} else {
			_renderer.drawFontTextureText(_px, _py, 10, 10, 0xccff3333, "NO SIGNAL");
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
	bool result = false;
	while (!result && SUCCEEDED(pEnumCat->Next(1, &moniker, NULL))) {
		string name;
		IPropertyBag* propBag = NULL;
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
					Poco::UnicodeConverter::toUTF8(wstring(&wname[0]), name);
					_log.information(Poco::format("device: %s", name));
				}
			}
			VariantClear(&varName);
			propBag->Release();
		}
		bool lookup = false;
		if (deviceName.empty()) {
			if (count == index) {
				lookup = true;
				deviceName = name;
			}
		} else if (deviceName == name) {
			lookup = true;
		}
		if (lookup) {
			hr = moniker->BindToObject(0, 0, IID_IBaseFilter, (void**)pBf);
			if (SUCCEEDED(hr)) {
				result = true;
				_log.information(Poco::format("lookup device: %s", name));
			}
		}
		moniker->Release();
		count++;
	}
	pEnumCat->Release();
	_log.information(Poco::format("device result: %s", string(result?"LOOKUP":"NOT FOUND")));
	return result;
}

bool CaptureScene::getPin(IBaseFilter* filter, IPin** pin, PIN_DIRECTION dir) {
    bool bFound = false;
    IEnumPins* pEnum = NULL;
    HRESULT hr = filter->EnumPins(&pEnum);
    if (SUCCEEDED(hr)) {
	    while (!bFound && pEnum->Next(1, pin, 0) == S_OK) {
	        PIN_DIRECTION PinDirThis;
	        (*pin)->QueryDirection(&PinDirThis);
	        if (dir == PinDirThis) {
				IPin* pToPin = NULL;
				hr = (*pin)->ConnectedTo(&pToPin);
				if (SUCCEEDED(hr)) {
					SAFE_RELEASE(pToPin);
					SAFE_RELEASE(*pin);
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
bool CaptureScene::getInPin(IBaseFilter* filter, IPin** pin) {
    return getPin(filter, pin, PINDIR_INPUT);
}

/* 指定したフィルタの出力ピンを返します */
bool CaptureScene::getOutPin(IBaseFilter* filter, IPin** pin) {
    return getPin(filter, pin, PINDIR_OUTPUT);
}

void CaptureScene::setWhiteBalance(IBaseFilter* src, bool autoFlag, long v) {
	IAMVideoProcAmp* procAmp = NULL;
	HRESULT hr = src->QueryInterface(&procAmp);
	if SUCCEEDED(hr) {
		// VideoProcAmp_Brightness
		// VideoProcAmp_Contrast
		// VideoProcAmp_Hue
		// VideoProcAmp_Saturation
		// VideoProcAmp_Sharpness
		// VideoProcAmp_Gamma
		// VideoProcAmp_ColorEnable
		// VideoProcAmp_WhiteBalance
		// VideoProcAmp_BacklightCompensation
		// VideoProcAmp_Gain
		long min, max, steppingDelta, defaultValue, capsFlags;
		hr = procAmp->GetRange(VideoProcAmp_WhiteBalance, &min, &max, &steppingDelta, &defaultValue, &capsFlags);
		string s = (capsFlags == VideoProcAmp_Flags_Auto)?"auto":"manual";
		_log.information(Poco::format("video whitebalance: %ld-%ld(%ld): %s", min, max, defaultValue, s));
		if (v == -100) v = defaultValue;
		if (autoFlag) {
			hr = procAmp->Set(VideoProcAmp_WhiteBalance, defaultValue, VideoProcAmp_Flags_Auto);
			if SUCCEEDED(hr) _log.information("video whitebalance: auto");
		} else {
			hr = procAmp->Set(VideoProcAmp_WhiteBalance, v, VideoProcAmp_Flags_Manual);
			if SUCCEEDED(hr) _log.information(Poco::format("video whitebalance: %ld", v));
		}
		SAFE_RELEASE(procAmp);
	}
}

void CaptureScene::setExposure(IBaseFilter* src, bool autoFlag, long v) {
	IAMCameraControl* cameraControl = NULL;
	HRESULT hr = src->QueryInterface(&cameraControl);
	if SUCCEEDED(hr) {
		// CameraControl_Pan
		// CameraControl_Tilt
		// CameraControl_Roll
		// CameraControl_Zoom
		// CameraControl_Exposure
		// CameraControl_Iris
		// CameraControl_Focus
		long min, max, steppingDelta, defaultValue, capsFlags;
		hr = cameraControl->GetRange(CameraControl_Exposure, &min, &max, &steppingDelta, &defaultValue, &capsFlags);
		string s = (capsFlags == CameraControl_Flags_Auto)?"auto":"manual";
		_log.information(Poco::format("camera exposure: %ld-%ld(%ld): %s", min, max, defaultValue, s));
		if (v == -100) v = defaultValue;
		if (autoFlag) {
			hr = cameraControl->Set(CameraControl_Exposure, defaultValue, CameraControl_Flags_Auto);
			if SUCCEEDED(hr) _log.information("camera exposure: auto");
		} else {
			hr = cameraControl->Set(CameraControl_Exposure, v, CameraControl_Flags_Manual);
			if SUCCEEDED(hr) _log.information(Poco::format("camera exposure: %ld", v));
		}
		SAFE_RELEASE(cameraControl);
	}
}

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

bool CaptureScene::changePlaylist() {
	if (_main) {
		bool prepared = false;
		string pl = _main->getStatus("prepared-playlist-id");
		if (pl == _detectedPlaylist) {
			prepared = true;
		} else {
			_log.information(Poco::format("change playlist: %s", _detectedPlaylist));
			prepared = _main->stackPrepareContent(_detectedPlaylist);
		}
		if (prepared) {
			for (int i = 0; i < 10; i++) {
				Poco::Thread::sleep(200);
				string pl = _main->getStatus("prepared-playlist-id");
				if (pl == _detectedPlaylist) {
					bool res = _main->switchContent();
					if (!res) {
						_log.warning("failed switch playlist");
					}
					return res;
				}
			}
			_log.warning("failed timeup switch playlist");
		} else {
			_log.warning(Poco::format("failed not prepared: %s", _detectedPlaylist));
		}
	}
	return false;
}