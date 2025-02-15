#pragma once
//=============================================================
// Renderer.h
// レンダラークラスの定義
//=============================================================

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <gdiplus.h>
#include <vector>
#include <queue>
#include <Poco/HashMap.h>
#include <Poco/Logger.h>
#include <Poco/Mutex.h>

#include "switch.h"
#include "Common.h"
#include "Configuration.h"
#include "FPSCounter.h"
#include <vfw.h>
#include <dsound.h>
#pragma comment(lib, "dsound.lib")
//#include <dxva2api.h>
//#pragma comment(lib, "dxva2.lib")


using std::string;
using std::wstring;
using std::vector;
using std::queue;


//=============================================================
// 依存するクラス
//=============================================================
class Scene;

/**
 * 頂点構造体
 */
struct VERTEX {
	float x, y, z;
	float rhw;
	D3DCOLOR color;
	float u, v;
};

#define VERTEX_FVF		(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)
#define VERTEX_SIZE		(sizeof(VERTEX))
#define VERTEX_COUNT	(256)

/**
 * キースキャン構造体
 */
struct KeyData {
	int keycode;
	bool shift;
	bool ctrl;
//	KeyData(int keycode_, bool shift_, bool ctrl_): keycode(keycode_), shift(shift_),ctrl(ctrl_) {}
};
typedef KeyData* KeyDataPtr;


/**
 * レンダラクラス.
 * レンダリングに関する機能を提供するユーティリティクラスです。シーンの管理も行います
 */
class Renderer
{
private:
	Poco::Logger& _log;
	Poco::FastMutex _lock; //mutable 修飾子外した
	Poco::FastMutex _sceneLock;
	//Poco::FastMutex _drawLock;
	bool _drawLock;
	Poco::FastMutex _deviceLock;

	HWND _hwnd;
	bool _postedQuit;
	int _exitCode;

	UINT _displayAdpters;
	UINT _maxTextureW;
	UINT _maxTextureH;

	ULONG_PTR _gdiToken;
	Gdiplus::GdiplusStartupInput _gdiSI;

	LPDIRECT3D9 _d3d;
	LPDIRECT3DDEVICE9 _device;
	D3DPRESENT_PARAMETERS* _presentParams;

	LPDIRECTSOUND _sound;

	UINT _deviceResetToken;
	//IDirect3DDeviceManager9* _devManager;

	int _mem;
	UINT _textureMem;
	UINT _availableTextureMem;

	DWORD _current;
	LPDIRECT3DSURFACE9 _backBuffer;
	LPDIRECT3DTEXTURE9 _captureTexture;

	FPSCounter _fpsCounter;

	Gdiplus::PrivateFontCollection* _fc;
//	Gdiplus::FontFamily* _multiByteFont;
	LPDIRECT3DTEXTURE9 _fontTexture;

	Poco::HashMap<string, LPDIRECT3DTEXTURE9> _cachedTextures;

	vector<string> _addresses;

	vector<Scene*> _scenes;
	Poco::HashMap<string, Scene*> _sceneMap;


	HDC _hdc;
	HFONT _hfontOLD;
	HFONT _hfont;

	bool _keyUpdated;
	int _keycode;
	bool _shift;
	bool _ctrl;

	vector<string> _addDrives;
	vector<string> _removeDrives;
	queue<string> _readyDrives;
	DWORD _lastDeviceChanged;

	std::map<string, string> _status;


	/**
	 * GDI+を使って文字列を描画します
	 */
	void drawText(const Gdiplus::FontFamily* ff, const int fontSize, const DWORD c1, const DWORD c2, const int w1, const DWORD c3, const int w2, const DWORD c4, const string& text, Gdiplus::Bitmap* bitmap, Gdiplus::Rect& rect) const;


	/**
	 * フォントテクスチャの生成
	 */
	void createFontTexture(const Gdiplus::FontFamily* fontFamily, const int fontSize);

	/** unitmaskからドライブレターへの変換 */
	//
	// この関数は以下のURLよりコピー＆改変
	// http://support.microsoft.com/kb/163503/ja
	//
	const string firstDriveFromMask(ULONG unitmask);

	// ボリュームをオープンしハンドルを取得します
	HANDLE openVolume(const string& driveLetter);

	// ボリュームハンドルを解放します
	BOOL closeVolume(HANDLE volume);

	// ボリュームをロックします
	BOOL lockVolume(HANDLE volume);

	// マウント解除
	BOOL dismountVolume(HANDLE volume);

	// メディアの強制排出設定
	BOOL preventRemovalOfVolume(HANDLE volume, BOOL preventRemoval);

	// メディアの排出
	BOOL autoEjectVolume(HANDLE volume);


public:
	Renderer();

	~Renderer();


	/**
	 * 3Dデバイス関連の初期化
	 */
	HRESULT initialize(HINSTANCE hInstance, HWND hWnd);

	/** ウィンドウハンドルを取得します */
	const HWND getWindowHandle() const;

	bool peekMessage();

	int getExitCode();

	/**
	 * UI用のメッセージを伝達します
	 */
	bool deliveryMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	/**
	 * 3Dデバイスを取得します
	 */
	const LPDIRECT3DDEVICE9 get3DDevice() const;

	/**
	 * 3Dデバイスを取得します
	 */
	const LPDIRECTSOUND getSoundDevice() const;

	/** テクスチャメモリ */
	const UINT getTextureMem() const;

	/** 残テクスチャメモリ */
	const UINT getAvailableTextureMem() const;

	/**
	 * キーボードデバイスを取得します
	 */
//	const LPDIRECTINPUTDEVICE8 getKeyboardDevice() const;

	void notifyKeyDown(const int keycode, const bool shift, const bool ctrl);

	void notifyKeyUp(const int keycode, const bool shift, const bool ctrl);


	/**
	 * マウスデバイスの取得します
	 */
//	const LPDIRECTINPUTDEVICE8 getMouseDevice() const;

	const int getSceneCount();

	void insertScene(const int i, const string name, Scene* scene);

	void addScene(const string name, Scene* scene);

	Scene* getScene(const string& name);

	void removeScene(const string& name);

	void drawLock();
	bool tryDrawLock();
	void drawUnlock();

	/**
	 * Sceneをレンダリングします
	 */
	void renderScene(const bool visibled, const LONGLONG current);

	const UINT getDisplayAdapters() const;

	const UINT getMaxTextureW() const;

	const UINT getMaxTextureH() const;

	/**
	 * テクスチャを生成
	 */
	const LPDIRECT3DTEXTURE9 createTexture(const int w, const int h, const D3DFORMAT format = D3DFMT_X8R8G8B8) const;

	/**
	 * 画像ファイルからテクスチャを生成
	 */
	const LPDIRECT3DTEXTURE9 createTexture(const string file) const;

	/**
	 * レンダリングターゲットを生成
	 */
	const LPDIRECT3DTEXTURE9 createRenderTarget(const int w, const int h, const D3DFORMAT format = D3DFMT_X8R8G8B8) const;

	const LPDIRECT3DSURFACE9 createLockableSurface(const int w, const int h, const D3DFORMAT format = D3DFMT_X8R8G8B8) const;

	const bool getRenderTargetData(LPDIRECT3DTEXTURE9 texture, LPDIRECT3DSURFACE9 surface) const;

	const bool updateRenderTargetData(LPDIRECT3DTEXTURE9 texture, LPDIRECT3DSURFACE9 surface) const;

	const bool colorFill(const LPDIRECT3DTEXTURE9 texture, const DWORD col) const;

	/**
	 * draw1()のレンダリング結果を取得します
	 */
	const LPDIRECT3DTEXTURE9 getCaptureTexture() const;

	// 線を描画
	void drawLine(const int x1, const int y1, const DWORD c1, const int x2, const int y2, const DWORD c2);

	/**
	 * テクスチャを指定位置に描画します
	 */
	void drawTexture(const int x, const int y, const LPDIRECT3DTEXTURE9 texture, const int flipMode, const D3DCOLOR c1 = 0xffffffff, const D3DCOLOR c2 = 0xffffffff, const D3DCOLOR c3 = 0xffffffff, const D3DCOLOR c4 = 0xffffffff) const;

	/**
	 * テクスチャを指定位置・範囲に描画します
	 */
	void drawTexture(const int x, const int y, const int w, const int h, const LPDIRECT3DTEXTURE9 texture, const int flipMode, const D3DCOLOR c1 = 0xffffffff, const D3DCOLOR c2 = 0xffffffff, const D3DCOLOR c3 = 0xffffffff, const D3DCOLOR c4 = 0xffffffff) const;

	/**
	 * テクスチャを指定位置・範囲・回転で描画します
	 */
	void drawTextureWithAngle(const int x, const int y, const int w, const int h, const int angle, const int cx, const int cy, const LPDIRECT3DTEXTURE9 texture, const int flipMode, const D3DCOLOR c1 = 0xffffffff, const D3DCOLOR c2 = 0xffffffff, const D3DCOLOR c3 = 0xffffffff, const D3DCOLOR c4 = 0xffffffff) const;

	/**
	 * テクスチャの指定部分を指定位置・範囲に描画します
	 */
	void drawTexture(const float dx, const float dy, const float dw, const float dh, const float sx, const float sy, const float sw, const float sh, const LPDIRECT3DTEXTURE9 texture, const int flipMode, const D3DCOLOR c1, const D3DCOLOR c2, const D3DCOLOR c3, const D3DCOLOR c4) const;

	/**
	 * テクスチャの指定部分を指定位置・範囲・回転に描画します
	 */
	void drawTextureWithAngle(const float dx, const float dy, const float dw, const float dh, const float sx, const float sy, const float sw, const float sh, const int angle, const int cx, const int cy, const LPDIRECT3DTEXTURE9 texture, const D3DCOLOR c1, const D3DCOLOR c2, const D3DCOLOR c3, const D3DCOLOR c4) const;

	/**
	 * エフェクトを生成します
	 */
	LPD3DXEFFECT createEffect(const string path);

	/**
	 * フォントファミリーの取得
	 */
	void getPrivateFontFamily(string fontName, Gdiplus::FontFamily** ff);

	/** フォントファミリ一覧取得 */
	void getPrivateFontFamilies(vector<string>& fonts);

	/**
	 * フォントファイル追加
	 */
	bool addPrivateFontFile(string file);

	/**
	 * GDIを使った文字列描画の開始
	 */
	bool beginFont(const wstring& fontFace, const Sint32 size);

	void drawFont(const Sint32 x, const Sint32 y, const COLORREF fontColor, const COLORREF backColor, const string& text) const;

	void endFont();

	bool copyTexture(LPDIRECT3DTEXTURE9 src, LPDIRECT3DTEXTURE9 dst);

	const LPDIRECT3DTEXTURE9 createTexturedText(const wstring& fontFamily, const int fontSize, const DWORD c1, const DWORD c2, const int w1, const DWORD c3, const int w2, const DWORD c4, const string& text, int clipH = -1) const;

	void drawFontTextureText(const int x, const int y, const int w, const int h, const D3DCOLOR col, const string s) const;

	void addCachedTexture(const string& name, const LPDIRECT3DTEXTURE9 texture);

	void removeCachedTexture(const string& name);

	const LPDIRECT3DTEXTURE9 getCachedTexture(const string& name) const;

	/** ドライブ追加の通知 */
	void addDrive(ULONG unitmask);

	/** ドライブ追加の通知 */
	void removeDrive(ULONG unitmask);

	/** 追加ドライブの有無 */
	bool hasAddDrives();

	/** デバイス変化の通知 */
	void deviceChanged();

	/** 準備ドライブ取得 */
	string popReadyDrive();

	// ボリュームのイジェクト
	BOOL ejectVolume(const string& driveLetter);

	/** ステータス情報の設定 */
	virtual void setStatus(const string& key, const string& value);

	/** ステータス情報の取得 */
	virtual const string getStatus(const string& key);

	/** ステータス情報の削除 */
	virtual void removeStatus(const string& key);

	/**
	 * 終了処理
	 */
	void finalize();
};

typedef Renderer* RendererPtr;
