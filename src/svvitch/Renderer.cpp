//=============================================================
// Renderer.cpp
// レンダラークラスの実装
//=============================================================
#define _WIN32_DCOM

#include "Renderer.h"

#include "Scene.h"
#include <Poco/File.h>
#include <Poco/string.h>
#include <Poco/format.h>
#include <Poco/UnicodeConverter.h>
#include <psapi.h>
#include <errors.h>


Renderer::Renderer(Configuration* conf): _log(Poco::Logger::get("")), _conf(conf), _d3d(NULL), _device(NULL), _backBuffer(NULL), _captureTexture(NULL), _sound(NULL), _fontTexture(NULL) 
{
}

Renderer::~Renderer() {
	finalize();
}

/**
 * 3Dデバイス関連の初期化
 */
HRESULT Renderer::initialize(HINSTANCE hInstance, HWND hWnd) {
	// COMの初期化
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
//	hr = CoInitialize(NULL);
	if (FAILED(hr)) {   
		vector<WCHAR> err(1024);
		DWORD res = AMGetErrorText(hr, &err[0], 1024);
		string utf8;
		Poco::UnicodeConverter::toUTF8(wstring(&err[0]), utf8);
		_log.warning(Poco::format("failed com initialize: %s", utf8));
	}

	GdiplusStartup(&_gdiToken, &_gdiSI, NULL);
	// Direct3D9オブジェクトの作成
	if ((_d3d = Direct3DCreate9(D3D_SDK_VERSION)) == 0) return E_FAIL;

	//利用できるアダプタ数を数える
	D3DCAPS9 caps;
	_d3d->GetDeviceCaps(0, D3DDEVTYPE_HAL, &caps);
	_displayAdpters = caps.NumberOfAdaptersInGroup;
	_maxTextureW = caps.MaxTextureWidth;
	_maxTextureH = caps.MaxTextureHeight;
	if (_conf->imageSplitWidth <= 0) {
		_conf->imageSplitWidth = _maxTextureW;
	}
	_log.information(Poco::format("display adapters: %u texture-max: %ux%u", _displayAdpters, _maxTextureW, _maxTextureH));
	string usePsize = (caps.FVFCaps & D3DFVFCAPS_PSIZE)?"true":"false";
	_log.information(Poco::format("set point size: %s", usePsize));

	// 現在のディスプレイモードを取得
	D3DDISPLAYMODE d3ddm;
	if (FAILED(_d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm))) {
		_log.warning("failed GetAdapterDisplayMode");
		return E_FAIL;
	}

	// デバイスのプレゼンテーションパラメータを初期化
	_presentParams = new D3DPRESENT_PARAMETERS[_displayAdpters];
	ZeroMemory(&_presentParams[0], sizeof(D3DPRESENT_PARAMETERS));
	if (_conf->fullsceen) {
		_presentParams[0].Windowed = FALSE;
//			_presentParams[0].FullScreen_RefreshRateInHz = _conf->rate;
		_presentParams[0].FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	} else {
		_presentParams[0].Windowed = TRUE;
		_presentParams[0].FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	}
	_presentParams[0].BackBufferWidth = _conf->mainRect.right;
	_presentParams[0].BackBufferHeight = _conf->mainRect.bottom;
	_presentParams[0].BackBufferFormat = d3ddm.Format;
	_presentParams[0].BackBufferCount = 1;
	_presentParams[0].SwapEffect = D3DSWAPEFFECT_DISCARD;
//	_presentParams[0].SwapEffect = D3DSWAPEFFECT_FLIP;
//	_presentParams[0].SwapEffect = D3DSWAPEFFECT_COPY;
	_presentParams[0].MultiSampleType = D3DMULTISAMPLE_NONE;
	_presentParams[0].MultiSampleQuality = 0;
	_presentParams[0].hDeviceWindow = hWnd;
	_presentParams[0].EnableAutoDepthStencil = FALSE;
	_presentParams[0].AutoDepthStencilFormat = D3DFMT_D16;
	_presentParams[0].Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	_presentParams[0].PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

	HWND hWnd2 = hWnd;
	for (int i = 1; i < _displayAdpters; i++) {
		//アダプタが2以上あればマルチヘッド用のD3DPRESENT_PARAMETERのデータを入力
		_presentParams[i] = _presentParams[0];
		WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0, 0, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"multihead", NULL};
		RegisterClassEx(&wc);
		std::wstring wtitle;
		Poco::UnicodeConverter::toUTF16(_conf->title, wtitle);
		if (SUCCEEDED(_d3d->GetAdapterDisplayMode(i, &d3ddm))) {
			hWnd2 = CreateWindow(wc.lpszClassName, wtitle.c_str(), WS_POPUP, 0, 0, d3ddm.Width, d3ddm.Height, NULL, NULL, wc.hInstance, NULL);
			_presentParams[i].BackBufferWidth = d3ddm.Width;
			_presentParams[i].BackBufferHeight = d3ddm.Height;
			_presentParams[i].FullScreen_RefreshRateInHz = d3ddm.RefreshRate;
			_conf->subRect.right = d3ddm.Width;
			_conf->subRect.bottom = d3ddm.Height;
		} else {
			hWnd2 = CreateWindow(wc.lpszClassName, wtitle.c_str(), WS_POPUP, 0, 0, _conf->subRect.right, _conf->subRect.bottom, NULL, NULL, wc.hInstance, NULL);
			_presentParams[i].FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
		}
		_presentParams[i].hDeviceWindow = hWnd2;
	}

	// ディスプレイアダプタを表すためのデバイスを作成
	DWORD flag = D3DCREATE_MULTITHREADED;
	if (_displayAdpters > 1) flag  |= D3DCREATE_ADAPTERGROUP_DEVICE;
	if (FAILED(_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, flag | D3DCREATE_HARDWARE_VERTEXPROCESSING, &_presentParams[0], &_device))) {
		// 上記の設定が失敗したら
		// 描画をハードウェアで行い、頂点処理はCPUで行なう
		if (FAILED(_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, flag | D3DCREATE_SOFTWARE_VERTEXPROCESSING, &_presentParams[0], &_device))) {
			// 上記の設定が失敗したら
			// 描画と頂点処理をCPUで行なう
			if (FAILED(_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hWnd, flag | D3DCREATE_SOFTWARE_VERTEXPROCESSING,& _presentParams[0], &_device))) {
				// 初期化失敗
				return E_FAIL;
			} else {
				_log.information("device: REF/SOFTWARE_VERTEXPROCESSING");
			}
		} else {
			_log.information("device: HAL/SOFTWARE_VERTEXPROCESSING");
		}
	} else {
		_log.information("device: HAL/HARDWARE_VERTEXPROCESSING");
	}

	// レンダリングターゲットの取得
	//	D3DSURFACE_DESC desc;
	//	HRESULT hr = _renderTarget->GetDesc(&desc);		
	//	if (SUCCEEDED(hr)) _log.information(Poco::format("render target: %ux%u %d", desc.Width, desc.Height, (int)desc.Format));
	_captureTexture = createRenderTarget(_conf->stageRect.right, _conf->stageRect.bottom);

	// サウンドデバイスの生成
	hr = DirectSoundCreate(NULL, &_sound, NULL);
	if (FAILED(hr)) {
		_log.warning("failed not initialize direct sound");
		return E_FAIL;
	}
	hr = _sound->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);
	if (FAILED(hr)) {
		_log.warning("failed not set cooperative level");
		return E_FAIL;
	}

	// フォント読込み
	_fc = new Gdiplus::PrivateFontCollection();
	Poco::File dir(".");
	vector<Poco::File> files;
	dir.list(files);
	for (vector<Poco::File>::iterator it = files.begin(); it != files.end(); it++) {
		Poco::File f = *it;
		int find = Poco::toLower(f.path()).find(".ttf");
		if (find >= 0 && find == f.path().length() - 4) {
			wstring wfile;
			Poco::UnicodeConverter::toUTF16(f.path(), wfile);
			_fc->AddFontFile(wfile.c_str());
		}
		find = Poco::toLower(f.path()).find(".ttc");
		if (find >= 0 && find == f.path().length() - 4) {
			wstring wfile;
			Poco::UnicodeConverter::toUTF16(f.path(), wfile);
			_fc->AddFontFile(wfile.c_str());
		}
	}
	Gdiplus::FontFamily ff[16];
	int num = 0;
	_fc->GetFamilies(16, ff, &num);
	for (int i = 0; i < num; i++) {
		WCHAR s[32] = L"";
		ff[i].GetFamilyName(s);
		string ffName;
		Poco::UnicodeConverter::toUTF8(s, ffName);
		_log.information(Poco::format("private font family: %s", ffName));
		if (ffName == _conf->asciiFont) createFontTexture(&ff[i], 32);
//		if (ffName == _conf->multiByteFont) _multiByteFont = ff[i].Clone();
	}

	_keycode = 0;
	_shift = false;
	_ctrl = false;

	return S_OK;
}

/**
 * 終了処理
 */
void Renderer::finalize() {
//	DXUTShutdown();
	for (vector<Scene*>::iterator it = _scenes.begin(); it != _scenes.end(); it++) {
		SAFE_DELETE(*it);
	}
	_scenes.clear();
	_sceneMap.clear();
	for (Poco::HashMap<string, LPDIRECT3DTEXTURE9>::Iterator it = _cachedTextures.begin(); it != _cachedTextures.end(); it++) {
		SAFE_RELEASE(it->second);
	}
	_cachedTextures.clear();

	SAFE_RELEASE(_fontTexture);
	SAFE_RELEASE(_captureTexture);
	SAFE_RELEASE(_backBuffer);
	SAFE_RELEASE(_device);
	SAFE_RELEASE(_d3d);
	SAFE_DELETE(_presentParams);
	SAFE_DELETE(_fc);
	Gdiplus::GdiplusShutdown(_gdiToken);
}

const HWND Renderer::getWindowHandle() const {
	return _hwnd;
}


/**
 * GDI+を使って文字列を描画します
 */
void Renderer::drawText(const Gdiplus::FontFamily* ff, const int fontSize, const DWORD c1, const DWORD c2, const int w1, const DWORD c3, const int w2, const DWORD c4, const string& text, Gdiplus::Bitmap* bitmap, Gdiplus::Rect& rect) const {
	int x = 0;
	int y = 0;
	if (rect.Width - rect.X != 0) {
		x = -rect.X;
	}
	if (rect.Height - rect.Y != 0) {
		y = -rect.Y;
	}

	Gdiplus::Graphics g(bitmap);
	g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
	Gdiplus::GraphicsPath path;
	std::wstring wtext;
	Poco::UnicodeConverter::toUTF16(text, wtext);
	path.AddString(wtext.c_str(), wcslen(wtext.c_str()), ff, Gdiplus::FontStyleRegular, fontSize, Gdiplus::Point(x, y), Gdiplus::StringFormat::GenericDefault());
	Gdiplus::Brush* foreBrush = NULL;
	if (c1 == c2) {
		foreBrush = new Gdiplus::SolidBrush(Gdiplus::Color(c1));
	} else {
		foreBrush = new Gdiplus::LinearGradientBrush(Gdiplus::Rect(0, 0, 1, rect.Height), Gdiplus::Color(c1), Gdiplus::Color(c2), Gdiplus::LinearGradientModeVertical);
	}
	Gdiplus::Pen* pen1 = NULL;
	if (w1 > 0) {
		Gdiplus::Color c1(c3);
		Gdiplus::SolidBrush brush(c1);
		pen1 = new Gdiplus::Pen(&brush, w1);
		pen1->SetLineJoin(Gdiplus::LineJoinRound);
		g.DrawPath(pen1, &path);
	}
	Gdiplus::Pen* pen2 = NULL;
	if (w2 > 0) {
		Gdiplus::Color c2(c4);
		Gdiplus::SolidBrush brush(c2);
		pen2 = new Gdiplus::Pen(&brush, w2);
		pen2->SetLineJoin(Gdiplus::LineJoinRound);
		g.DrawPath(pen2, &path);
	}
	g.FillPath(foreBrush, &path);

	// 描画領域を取得
	if (pen1) {
		// pen1のサイズでrectを取得
		path.Widen(pen1);
		path.GetBounds(&rect);
	} else if (pen2) {
		// pen2のサイズでrectを取得
		path.Widen(pen2);
		path.GetBounds(&rect);
	} else {
		path.GetBounds(&rect);
	}
	delete foreBrush;
	SAFE_DELETE(pen1);
	SAFE_DELETE(pen2);
	g.Flush();

//	{
//		UINT num;        // number of image encoders
//		UINT size;       // size, in bytes, of the image encoder array
//		ImageCodecInfo* pImageCodecInfo;
//		GetImageEncodersSize(&num, &size);
//		pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
//		GetImageEncoders(num, size, pImageCodecInfo);
//		for (int i = 0; i < num ; i ++) {
//			if (!wcscmp(pImageCodecInfo[i].MimeType, L"image/png")) {
//				bitmap.Save(L"test.png", &pImageCodecInfo[i].Clsid);
//				break;
//			}
//		}
//		free(pImageCodecInfo);
//	}
}

/**
 * フォントテクスチャの生成
 */
void Renderer::createFontTexture(const Gdiplus::FontFamily* fontFamily, const int fontSize) {
	_fontTexture = createTexture(512, 512, D3DFMT_A8R8G8B8);
	if (_fontTexture) {
		colorFill(_fontTexture, 0);
		D3DLOCKED_RECT lockRect;
		HRESULT hr = _fontTexture->LockRect(0, &lockRect, NULL, 0);
		if (FAILED(hr)) {
			SAFE_RELEASE(_fontTexture);
		} else {
			Gdiplus::Bitmap bitmap(512, 512, lockRect.Pitch, PixelFormat32bppARGB, (BYTE*)lockRect.pBits);
			Gdiplus::Graphics g(&bitmap);
			g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
			g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
			Gdiplus::Font font(fontFamily, fontSize, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
			Gdiplus::StringFormat format;
			format.SetAlignment(Gdiplus::StringAlignmentCenter);
			Gdiplus::SolidBrush brush(Gdiplus::Color::White);
			WCHAR ascii[2] = L"";
			for (int i = 0x20; i <= 0xff; i++) {
				int x = (i % 16) * 32;
				int y = (i / 16) * 32;
				ascii[0] = i;
				g.DrawString(ascii, 1, &font, Gdiplus::RectF(x, y - 3, 32, 32 + 3), &format, &brush);
			}
			g.Flush();
			_fontTexture->UnlockRect(0);
			_log.information("create font texture");
		}
	} else {
		_log.warning("failed create font texture");
	}
}


/**
 * UI用のメッセージを伝達します
 */
bool Renderer::deliveryMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CREATE:
		break;
	case WM_MOUSEMOVE:
		break;
	case WM_LBUTTONDOWN:
		//押す
		break;
	case WM_LBUTTONUP:
		//離す
		break;
	case WM_LBUTTONDBLCLK:
		//ダブルクリック
		break;
	case WM_RBUTTONDOWN:
		break;
	case WM_RBUTTONUP:
		break;
	case WM_RBUTTONDBLCLK:
		break;
	}
	return false;
}

/**
 * 設定オブジェクトを取得します
 */
Configuration* Renderer::config() const {
	return _conf;
}

/**
 * 3Dデバイスを取得します
 */
const LPDIRECT3DDEVICE9 Renderer::get3DDevice() const {
	return _device;
}

/**
 * 3Dデバイスを取得します
 */
const LPDIRECTSOUND Renderer::getSoundDevice() const {
	return _sound;
}

/**
 * キーボードデバイスを取得します
 */
void Renderer::notifyKeyDown(const int keycode, const bool shift, const bool ctrl) {
	_keycode = keycode;
	_shift = shift;
	_ctrl = ctrl;
	_keyUpdated = true;
}

void Renderer::notifyKeyUp(const int keycode, const bool shift,const  bool ctrl) {
}

const int Renderer::getSceneCount() {
	Poco::ScopedLock<Poco::FastMutex> lock(_sceneLock);
	return _scenes.size();
}

void Renderer::insertScene(const int i, const string name, Scene* scene) {
	Poco::ScopedLock<Poco::FastMutex> lock(_sceneLock);
	int j = 0;
	for (vector<Scene*>::iterator it = _scenes.begin(); it != _scenes.end(); it++) {
		if (i <= j) {
			_scenes.insert(it, scene);
			_sceneMap[name] = scene;
			return;
		}
		j++;
	}
	addScene(name, scene);
}

void Renderer::addScene(const string name, Scene* scene) {
	Poco::ScopedLock<Poco::FastMutex> lock(_sceneLock);
	_scenes.push_back(scene);
	_sceneMap[name] = scene;
}

Scene* Renderer::getScene(const string& name) {
	Poco::ScopedLock<Poco::FastMutex> lock(_sceneLock);
	Poco::HashMap<string, Scene*>::Iterator it = _sceneMap.find(name);
	if (it != _sceneMap.end()) {
		return it->second;
	}
	return NULL;
}

void Renderer::removeScene(const string& name) {
	Poco::ScopedLock<Poco::FastMutex> lock(_sceneLock);
	Poco::HashMap<string, Scene*>::Iterator i = _sceneMap.find(name);
	if (i != _sceneMap.end()) {
		for (vector<Scene*>::iterator it = _scenes.begin(); it != _scenes.end(); it++) {
			if (i->second == *it) {
				_sceneMap.erase(name);
				_scenes.erase(it);
				_log.information(Poco::format("*remove scene: %s", name));
				break;
			}
		}
	}
}

/**
 *  Sceneをレンダリングします
 */
void Renderer::renderScene(const DWORD current) {
	int keycode = 0;
	bool shift = false;
	bool ctrl = false;
	if (_keyUpdated) {
		_keyUpdated = false;
		keycode = _keycode;
		shift = _shift;
		ctrl = _ctrl;
	}
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_sceneLock);
		for (vector<Scene*>::iterator it = _scenes.begin(); it != _scenes.end(); it++) {
			Scene* scene = (*it);
			scene->notifyKey(keycode, shift, ctrl);
			scene->process();
		}
	}

	_device->SetFVF(VERTEX_FVF);
	_device->SetRenderState(D3DRS_ZENABLE, false);
	_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	LPDIRECT3DSWAPCHAIN9 swapChain1 = NULL;
	LPDIRECT3DSWAPCHAIN9 swapChain2 = NULL;
	HRESULT hr = _device->GetSwapChain(0, &swapChain1);
	if (SUCCEEDED(hr)) {
		LPDIRECT3DSURFACE9 backBuffer = NULL; //バックバッファ
		hr = swapChain1->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
		hr = _device->SetRenderTarget(0, backBuffer);
		SAFE_RELEASE(backBuffer);
	}

	if (FAILED(_device->Clear(0, NULL, D3DCLEAR_TARGET, D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f),  1.0f, 0))) {
		return;
	}

	// 描画1
	if (SUCCEEDED(_device->BeginScene())) {
		Poco::ScopedLock<Poco::FastMutex> lock(_sceneLock);
		for (vector<Scene*>::iterator it = _scenes.begin(); it != _scenes.end(); it++) {
			(*it)->draw1();
		}
		_device->EndScene();

		if (false) { 
			LPDIRECT3DSURFACE9 src;
			_device->GetRenderTarget(0, &src);
			wstring wfile;
			Poco::UnicodeConverter::toUTF16(string("snapshot.png"), wfile);
			D3DXSaveSurfaceToFile(wfile.c_str(), D3DXIFF_PNG, src, NULL, NULL);
			SAFE_RELEASE(src);
		}
	}

	if (_displayAdpters > 1) {
		swapChain1->Present(NULL, NULL, NULL, NULL, 0);

		if (_captureTexture) {
			LPDIRECT3DSURFACE9 backBuffer = NULL;
			HRESULT hr = _device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
			if (SUCCEEDED(hr)) {
				LPDIRECT3DSURFACE9 dst = NULL;
				hr = _captureTexture->GetSurfaceLevel(0, &dst);
				if (SUCCEEDED(hr)) {
					_device->StretchRect(backBuffer, &(_conf->stageRect), dst, NULL, D3DTEXF_LINEAR); // D3DTEXF_NONE
					SAFE_RELEASE(dst);
				} else {
					_log.warning("failed get capture surface");
				}
				SAFE_RELEASE(backBuffer);
			} else {
				_log.warning("failed get back buffer");
			}
		}

		hr = _device->GetSwapChain(1, &swapChain2);
		if (SUCCEEDED(hr)) {
			LPDIRECT3DSURFACE9 backBuffer = NULL; //バックバッファ
			hr = swapChain2->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
			hr = _device->SetRenderTarget(0, backBuffer);
			SAFE_RELEASE(backBuffer);

			if (FAILED(_device->Clear(0, NULL, D3DCLEAR_TARGET, D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f),  1.0f, 0))) {
				//
			}
		}
	}

	// 描画2
	_device->GetRenderTarget(0, &_backBuffer);
	if (SUCCEEDED(_device->BeginScene())) {
		Poco::ScopedLock<Poco::FastMutex> lock(_sceneLock);
		for (vector<Scene*>::iterator it = _scenes.begin(); it != _scenes.end(); it++) {
			(*it)->draw2();
		}

		MEMORYSTATUS ms;
		ms.dwLength = sizeof(MEMORYSTATUS);
		GlobalMemoryStatus(&ms);
		int availMem = ms.dwAvailPhys / 1024 / 1024;

		PROCESS_MEMORY_COUNTERS pmc = {0};
		GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(PROCESS_MEMORY_COUNTERS));
		int mem = pmc.WorkingSetSize / 1024 / 1024;

		string time = Poco::format("%02lu:%02lu:%02lu.%03lu", current / 3600000, current / 60000 % 60, current / 1000 % 60, current % 1000);
		Uint32 vram = _device->GetAvailableTextureMem() / 1024 / 1024;
		string memory = Poco::format("ram:%03dMB/avail:%03dMB vram:%03luMB", mem, availMem, vram);
//		string mouse = Poco::format("mouse: %04ld,%03ld,%03ld", _dims.lX, _dims.lY, _dims.lZ);
		drawFontTextureText(0, _conf->subRect.bottom - 20, 12, 16, 0xffcccccc, Poco::format("FPS:%03lu %s %s", _fpsCounter.getFPS(), time, memory));

		_device->EndScene();
	}
	if (_displayAdpters > 1) {
		swapChain2->Present(NULL, NULL, NULL, NULL, 0);

	} else {
		if (FAILED(_device->Present(0, 0, 0, 0))) {
			_device->Reset(&_presentParams[0]);
		}

		if (_captureTexture) {
			LPDIRECT3DSURFACE9 backBuffer = NULL;
			HRESULT hr = _device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
			if (SUCCEEDED(hr)) {
				LPDIRECT3DSURFACE9 dst = NULL;
				hr = _captureTexture->GetSurfaceLevel(0, &dst);
				if (SUCCEEDED(hr)) {
					_device->StretchRect(backBuffer, NULL, dst, NULL, D3DTEXF_NONE);
					SAFE_RELEASE(dst);
				} else {
					_log.warning("failed get capture surface");
				}
				SAFE_RELEASE(backBuffer);
			} else {
				_log.warning("failed get back buffer");
			}
		}
	}

	SAFE_RELEASE(_backBuffer);
	SAFE_RELEASE(swapChain1);
	SAFE_RELEASE(swapChain2);
	_fpsCounter.count();
}

const UINT Renderer::getDisplayAdapters() const {
	return _displayAdpters;
}

const UINT Renderer::getMaxTextureW() const {
	return _maxTextureW;
}

const UINT Renderer::getMaxTextureH() const {
	return _maxTextureH;
}

/**
 * テクスチャを生成
 */
const LPDIRECT3DTEXTURE9 Renderer::createTexture(const int w, const int h, const D3DFORMAT format) const {
	LPDIRECT3DTEXTURE9 texture = NULL;
	HRESULT hr = _device->CreateTexture(w, h, 1, 0, format, D3DPOOL_MANAGED, &texture, NULL);
	if (SUCCEEDED(hr)) {
//		D3DSURFACE_DESC desc;
//		hr = texture->GetLevelDesc(0, &desc);		
//		if (SUCCEEDED(hr)) _log.information(Poco::format("create texture: %ux%u", desc.Width, desc.Height));
	} else if (D3DERR_INVALIDCALL == hr) {
		_log.warning(Poco::format("failed create texture: %dx%d", w, h));
	} else if (D3DERR_OUTOFVIDEOMEMORY == hr) {
		_log.warning(Poco::format("failed create texture(out of videomemory): %dx%d", w, h));
	} else if (E_OUTOFMEMORY == hr) {
		_log.warning(Poco::format("failed create texture(out of memory): %dx%d", w, h));
	}
	return texture;
}

/**
 * 画像ファイルからテクスチャを生成
 */
const LPDIRECT3DTEXTURE9 Renderer::createTexture(const string file) const {
	wstring wfile;
	Poco::UnicodeConverter::toUTF16(file, wfile);
	D3DXIMAGE_INFO info;
	HRESULT hr = D3DXGetImageInfoFromFile(wfile.c_str(), &info);
	LPDIRECT3DTEXTURE9 texture = NULL;
	if (SUCCEEDED(hr)) {
		hr = D3DXCreateTextureFromFileEx(_device, wfile.c_str(), info.Width, info.Height, 1, 0, info.Format, D3DPOOL_DEFAULT, D3DX_FILTER_NONE, D3DX_DEFAULT, 0, NULL, NULL, &texture);
		if (FAILED(hr)) {
			_log.warning(Poco::format("failed create texture: %s", file));
		}
	}
	return texture;
}

/**
 * レンダリングターゲットを生成
 */
const LPDIRECT3DTEXTURE9 Renderer::createRenderTarget(const int w, const int h, const D3DFORMAT format) const {
	LPDIRECT3DTEXTURE9 texture = NULL;
	HRESULT hr = _device->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, format, D3DPOOL_DEFAULT, &texture, NULL);
	if (SUCCEEDED(hr)) {
		D3DSURFACE_DESC desc;
		hr = texture->GetLevelDesc(0, &desc);
		if (SUCCEEDED(hr)) _log.information(Poco::format("create render target texture: %ux%u", desc.Width, desc.Height));
	} else if (D3DERR_INVALIDCALL == hr) {
		_log.warning(Poco::format("failed create texture: %dx%d", w, h));
	} else if (D3DERR_OUTOFVIDEOMEMORY == hr) {
		_log.warning(Poco::format("failed create texture(out of videomemory): %dx%d", w, h));
	} else if (E_OUTOFMEMORY == hr) {
		_log.warning(Poco::format("failed create texture(out of memory): %dx%d", w, h));
	}
	return texture;
}

const LPDIRECT3DSURFACE9 Renderer::createLockableSurface(const int w, const int h, const D3DFORMAT format) const {
	LPDIRECT3DSURFACE9 surface = NULL;
	HRESULT hr = _device->CreateOffscreenPlainSurface(w, h, format, D3DPOOL_SYSTEMMEM, &surface, NULL);
	return surface;
}

const bool Renderer::getRenderTargetData(LPDIRECT3DTEXTURE9 texture, LPDIRECT3DSURFACE9 surface) const {
	LPDIRECT3DSURFACE9 target;
	HRESULT hr = texture->GetSurfaceLevel(0, &target);
	if (SUCCEEDED(hr)) {
		hr = _device->GetRenderTargetData(target, surface);
		SAFE_RELEASE(target);
		return SUCCEEDED(hr);
	}
	return false;
}

const bool Renderer::updateRenderTargetData(LPDIRECT3DTEXTURE9 texture, LPDIRECT3DSURFACE9 surface) const {
	LPDIRECT3DSURFACE9 target;
	HRESULT hr = texture->GetSurfaceLevel(0, &target);
	if (SUCCEEDED(hr)) {
		hr = _device->UpdateSurface(surface, NULL, target, NULL);
		SAFE_RELEASE(target);
		return SUCCEEDED(hr);
	}
	return false;
}

const bool Renderer::colorFill(const LPDIRECT3DTEXTURE9 texture, const DWORD col) const {
	LPDIRECT3DSURFACE9 dst = NULL;
	HRESULT hr = texture->GetSurfaceLevel(0, &dst);
	if (SUCCEEDED(hr)) {
		hr = _device->ColorFill(dst, NULL, col);
		SAFE_RELEASE(dst);
	}
	return SUCCEEDED(hr);
}

/**
 * 全てのScene.draw1()のレンダリング結果を取得します
 */
const LPDIRECT3DTEXTURE9 Renderer::getCaptureTexture() const {
	return _captureTexture;
}

/**
 * テクスチャを指定位置に描画します
 */
void Renderer::drawTexture(const int x, const int y, const LPDIRECT3DTEXTURE9 texture, const int flipMode, const D3DCOLOR c1, const D3DCOLOR c2, const D3DCOLOR c3, const D3DCOLOR c4) const {
	Uint32 tw = 0, th = 0;
	if (texture) {
		D3DSURFACE_DESC desc;
		HRESULT hr = texture->GetLevelDesc(0, &desc);
		tw = desc.Width;
		th = desc.Height;
	}
	drawTexture(x, y, tw, th, texture, flipMode, c1, c2, c3, c4);
}

/**
 * テクスチャを指定位置･サイズで描画します
 */
void Renderer::drawTexture(const int x, const int y, const int w, const int h, const LPDIRECT3DTEXTURE9 texture, const int flipMode, const D3DCOLOR c1, const D3DCOLOR c2, const D3DCOLOR c3, const D3DCOLOR c4) const {
	float u1, u2, v1, v2;
	switch (flipMode) {
		case 1:
			u1 = F(1); u2 = F(0);
			v1 = F(0); v2 = F(1);
			break;
		default:
			u1 = F(0); u2 = F(1);
			v1 = F(0); v2 = F(1);
	}
	float x1 = F(x     - 0.5);
	float y1 = F(y     - 0.5);
	float x2 = F(x + w - 0.5);
	float y2 = F(y + h - 0.5);

	_device->SetTexture(0, texture);
	VERTEX dst[] =
		{
			{x1, y1, 0.0f, 1.0f, c1, u1, v1},
			{x2, y1, 0.0f, 1.0f, c2, u2, v1},
			{x1, y2, 0.0f, 1.0f, c3, u1, v2},
			{x2, y2, 0.0f, 1.0f, c4, u2, v2}
		};
	_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, VERTEX_SIZE);
	_device->SetTexture(0, NULL);
}

/**
 * テクスチャの指定部分を指定位置・範囲に描画します
 */
void Renderer::drawTexture(const float dx, const float dy, const float dw, const float dh, const float sx, const float sy, const float sw, const float sh, const LPDIRECT3DTEXTURE9 texture, const D3DCOLOR c1, const D3DCOLOR c2, const D3DCOLOR c3, const D3DCOLOR c4) const {
	float u1, v1, u2, v2;
	if (texture) {
		D3DSURFACE_DESC desc;
		HRESULT hr = texture->GetLevelDesc(0, &desc);
		u1 = sx / desc.Width;
		v1 = sy / desc.Height;
		u2 = (sx + sw) / desc.Width;
		v2 = (sy + sh) / desc.Height;
	}

	_device->SetTexture(0, texture);
	VERTEX dst[] =
		{
			{dx      - 0.5, dy      - 0.5, 0.0f, 1.0f, c1, u1, v1},
			{dx + dw - 0.5, dy      - 0.5, 0.0f, 1.0f, c2, u2, v1},
			{dx      - 0.5, dy + dh - 0.5, 0.0f, 1.0f, c3, u1, v2},
			{dx + dw - 0.5, dy + dh - 0.5, 0.0f, 1.0f, c4, u2, v2}
		};
	_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, VERTEX_SIZE);
	_device->SetTexture(0, NULL);
}

/**
 * GDIを使った文字列描画の開始
 */
bool Renderer::beginFont(const wstring& fontFace, const Sint32 size) {
	if (_backBuffer) {
		HRESULT hr = _backBuffer->GetDC(&_hdc);
		if (FAILED(hr)) {
			_log.warning("failed get backbuffer HDC");
			return false;
		}

		_hfont = CreateFont(
			size,						// フォント高さ
			0,							// 文字幅
			0,							// テキストの角度	
			0,							// ベースラインとｘ軸との角度
			FW_BOLD,					// フォントの重さ（太さ）
			false,						// イタリック体
			false,						// アンダーライン
			false,						// 打ち消し線
			SHIFTJIS_CHARSET,			// 文字セット
			OUT_TT_PRECIS,				// 出力精度
			CLIP_DEFAULT_PRECIS,		// クリッピング精度
			PROOF_QUALITY,				// 出力品質
			FIXED_PITCH | FF_MODERN,	// ピッチとファミリー
			fontFace.c_str());			// 書体名
		if (!_hfont) {
			_log.warning("failed create HFONT");
			return false;
		}
		// フォント設定
		_hfontOLD = (HFONT)SelectObject(_hdc, _hfont);

		return true;
	}
	return false;
}

void Renderer::drawFont(const Sint32 x, const Sint32 y, const COLORREF fontColor, const COLORREF backColor, const string& text) const {
	if (!_hdc) return;
	SetBkColor(_hdc, backColor);	// 背景色
	SetBkMode(_hdc, OPAQUE);
	SetTextColor(_hdc, fontColor);	// フォント色

	// フォント描画
	wstring wtext;
	Poco::UnicodeConverter::toUTF16(text, wtext);
	TextOut(_hdc, x, y, wtext.c_str(), wtext.length());
}

void Renderer::endFont() {
	if (!_hdc) return;
	SelectObject(_hdc, _hfontOLD);
	DeleteObject(_hfont);
	if (_backBuffer) {
		HRESULT hr = _backBuffer->ReleaseDC(_hdc);
	}
	_hdc = NULL;
	_hfont = NULL;
	_hfontOLD = NULL;
}

const LPDIRECT3DTEXTURE9 Renderer::createTexturedText(const wstring& fontFamily, const int fontSize, const DWORD c1, const DWORD c2, const int w1, const DWORD c3, const int w2, const DWORD c4, const string& text, int clipH) const {
	WCHAR s[32] = L"";
	Gdiplus::FontFamily* ff = NULL;
	if (fontFamily.empty()) {
		// フォント指定が無い場合はデフォルトのフォントを検索
		Gdiplus::FontFamily temp[16];
		int num = 0;
		_fc->GetFamilies(16, temp, &num);
		for (int i = 0; i < num; i++) {
			temp[i].GetFamilyName(s);
			string name;
			Poco::UnicodeConverter::toUTF8(s, name);
			if (_conf->multiByteFont == name) ff = temp[i].Clone();
		}
	}

	if (!ff) {
		wstring fontName = fontFamily;
		if (fontName.empty()) fontName = _conf->defaultFont;
		Gdiplus::Font f(fontName.c_str(), fontSize);
		Gdiplus::FontFamily temp;
		f.GetFamily(&temp);
		ff = temp.Clone();
		ff->GetFamilyName(s);
		string name;
		Poco::UnicodeConverter::toUTF8(s, name);
//		_log.information(Poco::format("use font: %s", name));
	}

	// 文字列レンダリング後サイズの確認
	Gdiplus::Rect rect(0, 0, 0, 0);
	{
		Gdiplus::Bitmap bitmap(1, 1, PixelFormat32bppARGB);
		drawText(ff, fontSize, c1, c2, w1, c3, w2, c4, text, &bitmap, rect);
	}

	// 最終的な描画処理
	LPDIRECT3DTEXTURE9 texture = NULL;
	{
		int x = rect.X;
		int y = rect.Y;
		int w = rect.Width;
		int h = rect.Height;
		rect.Width = w - x;		// rectの領域をx/y=0で作り直す
		rect.Height = h - y;	// ただしx/yはクリアせずそのまま引き渡すことで、biasとして使用する
//		_log.information(Poco::format("bitmap(%d,%d %dx%d): %s", x, y, w, h, text));
		if (clipH < 0) clipH = h;
		texture = createTexture(w, clipH, D3DFMT_A8R8G8B8);
		if (!texture) {
			// テクスチャが生成できない
			SAFE_DELETE(ff);
			return NULL;
		}
		D3DSURFACE_DESC desc;
		HRESULT hr = texture->GetLevelDesc(0, &desc);
		Uint32 tw = desc.Width;
		Uint32 th = desc.Height;
//		_log.information(Poco::format("textured text: %lux%lu %s", tw, th, text));
		if (w > tw) {
			// テクスチャの幅の方が小さい場合、テクスチャを折返しで作る
			SAFE_RELEASE(texture);
			Gdiplus::Bitmap bitmap(w, h, PixelFormat32bppARGB);
			drawText(ff, fontSize, c1, c2, w1, c3, w2, c4, text, &bitmap, rect);
			texture = createTexture(tw, th * ((w + tw - 1) / tw), D3DFMT_A8R8G8B8);
			colorFill(texture, D3DCOLOR_ARGB(0, 0, 0, 0));
			HRESULT hr = texture->GetLevelDesc(0, &desc);
			tw = desc.Width;
			th = desc.Height;
			_log.debug(Poco::format("texture: %lux%lu", tw, th));
			D3DLOCKED_RECT lockRect;
			hr = texture->LockRect(0, &lockRect,NULL, 0);
			if (SUCCEEDED(hr)) {
				Gdiplus::Bitmap dst(tw, th, lockRect.Pitch, PixelFormat32bppARGB, (BYTE*)lockRect.pBits);
				Gdiplus::Graphics g(&dst);
				int y = 0;
				for (int x = 0; x < w; x+=tw) {
					Gdiplus::Rect rect(0, y, tw, 32);
					g.SetClip(rect);
					g.DrawImage(&bitmap, -x, y);
					y += 32;
				}
				g.Flush();
				texture->UnlockRect(0);
				_log.debug(Poco::format("draw texture(with turns): %lux%lu", tw, th));
//				_w = 1024;
			}

		} else {
			// 折返し無し
			colorFill(texture, D3DCOLOR_ARGB(0, 0, 0, 0));
			D3DLOCKED_RECT lockRect;
			HRESULT hr = texture->LockRect(0, &lockRect,NULL, 0);
			if (SUCCEEDED(hr)) {
				Gdiplus::Bitmap bitmap(tw, th, lockRect.Pitch, PixelFormat32bppARGB, (BYTE*)lockRect.pBits);
				drawText(ff, fontSize, c1, c2, w1, c3, w2, c4, text, &bitmap, rect);
				texture->UnlockRect(0);
				_log.debug(Poco::format("draw text texture: %lux%lu", tw, th));
//				_w = rect.Width;
			}
		}
	}
	SAFE_DELETE(ff);
	return texture;
}

void Renderer::drawFontTextureText(const int x, const int y, const int w, const int h, const D3DCOLOR col, const string s) const {
	if (_fontTexture) {
		D3DSURFACE_DESC desc;
		HRESULT hr = _fontTexture->GetLevelDesc(0, &desc);
		UINT tw = desc.Width;
		UINT th = desc.Height;
		_device->SetTexture(0, _fontTexture);
		for (int i = 0; i < s.length(); i++) {
			int dx = x + i * w;
			int a = s[i];
			float u1 = F((a % 16) * 32) / tw;
			float v1 = F((a / 16) * 32) / th;
			float u2 = F((a % 16) * 32 + 31) / tw;
			float v2 = F((a / 16) * 32 + 31) / th;

			{
				VERTEX dst[] =
					{
						{F(dx     + 2), F(y     + 2), 0.0f, 1.0f, 0x66000000, u1, v1},
						{F(dx + w + 2), F(y     + 2), 0.0f, 1.0f, 0x66000000, u2, v1},
						{F(dx     + 2), F(y + h + 2), 0.0f, 1.0f, 0x66000000, u1, v2},
						{F(dx + w + 2), F(y + h + 2), 0.0f, 1.0f, 0x66000000, u2, v2}
					};
				hr = _device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, sizeof(VERTEX));
			}
			VERTEX dst[] =
				{
					{F(dx     - 0.5), F(y     - 0.5), 0.0f, 1.0f, col, u1, v1},
					{F(dx + w - 0.5), F(y     - 0.5), 0.0f, 1.0f, col, u2, v1},
					{F(dx     - 0.5), F(y + h - 0.5), 0.0f, 1.0f, col, u1, v2},
					{F(dx + w - 0.5), F(y + h - 0.5), 0.0f, 1.0f, col, u2, v2}
				};
			hr = _device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, sizeof(VERTEX));
		}
		_device->SetTexture(0, NULL);

	} else {
//		beginFont(L"ＭＳ ゴシック", h);
//		drawFont(x, y, col & 0xffffff, 0x333333, s);
//		endFont();
	}
}

void Renderer::addCachedTexture(const string& name, const LPDIRECT3DTEXTURE9 texture) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	Poco::HashMap<string, LPDIRECT3DTEXTURE9>::Iterator it = _cachedTextures.find(name);
	if (it != _cachedTextures.end()) {
		_log.information(Poco::format("texture already registed: %s", name));
		SAFE_RELEASE(it->second);
		_cachedTextures.erase(name);
	}
	_cachedTextures[name] = texture;
}

void Renderer::removeCachedTexture(const string& name) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	Poco::HashMap<string, LPDIRECT3DTEXTURE9>::Iterator it = _cachedTextures.find(name);
	if (it != _cachedTextures.end()) {
		SAFE_RELEASE(it->second);
		_cachedTextures.erase(name);
	}
}

const LPDIRECT3DTEXTURE9 Renderer::getCachedTexture(const string& name) const {
//	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	Poco::HashMap<string, LPDIRECT3DTEXTURE9>::ConstIterator it = _cachedTextures.find(name);
	if (it != _cachedTextures.end()) {
		return it->second;
	}
	return NULL;
}

