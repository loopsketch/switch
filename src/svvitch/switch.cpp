//
// switch.cpp
// アプリケーションの実装
//

#include <winsock2.h>
#include <windows.h>
#include <psapi.h>
#include <gdiplus.h>
#include <Dbt.h>

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/Exception.h>
#include <Poco/Channel.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/FileChannel.h>
#include <Poco/FormattingChannel.h>
#include <Poco/format.h>
#include <Poco/Logger.h>
#include <Poco/Thread.h>
#include <Poco/Util/XMLConfiguration.h>
#include <Poco/UnicodeConverter.h>
#include <Poco/PatternFormatter.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/ServerSocket.h>

#include "switch.h"
#include "Renderer.h"
#include "CaptureScene.h"
#include "MainScene.h"
#include "UserInterfaceScene.h"
#include "Workspace.h"
#include "WebAPI.h"
#include "ui/UserInterfaceManager.h"

extern "C" {
#define inline _inline
#include <libavutil/avstring.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
}

#ifndef _DEBUG
#include <omp.h>
#endif

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>                // 最後にインクルードしたほうがいい気がする。C++の場合。標準ヘッダの中にnewとかあると変になる？？
#ifdef _DEBUG
#define new ::new(_NORMAL_BLOCK,__FILE__,__LINE__)     // これが重要
#endif

using Poco::AutoPtr;
using Poco::File;
using Poco::Util::XMLConfiguration;
using Poco::XML::Document;
using Poco::XML::Element;


static TCHAR clsName[] = TEXT("switchClass"); // クラス名

static Poco::Channel* _logFile;
static Poco::Logger& _log = Poco::Logger::get("");
static Configuration _conf;

static RendererPtr _renderer;
static ui::UserInterfaceManagerPtr _uim;

static std::string _interruptFile;


//-------------------------------------------------------------
// アプリケーションのエントリポイント
// 引数
//		hInstance     : 現在のインスタンスのハンドル
//		hPrevInstance : 以前のインスタンスのハンドル
//		lpCmdLine	  : コマンドラインパラメータ
//		nCmdShow	  : ウィンドウの表示状態
// 戻り値
//		成功したら0以外の値
//-------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//	_CrtSetBreakAlloc(4141);
//	_CrtSetBreakAlloc(3627);
//	_CrtSetBreakAlloc(7048);
#endif
#if defined(DEBUG) | defined(_DEBUG)
#endif
#ifdef _DEBUG
//_CrtDumpMemoryLeaks();
#endif

	if (!guiConfiguration()) return 0;

	HWND hWnd;
	MSG msg;
	// ウィンドウクラスの初期化
	WNDCLASSEX wcex = {
		sizeof(WNDCLASSEX),				// この構造体のサイズ
		CS_DBLCLKS,						// ウインドウのスタイル(default)
		WindowProc,						// メッセージ処理関数の登録
		0,								// 通常は使わないので常に0
		0,								// 通常は使わないので常に0
		hInstance,						// インスタンスへのハンドル
		NULL,							// アイコン（なし）
		LoadCursor(NULL, IDC_ARROW),	// カーソルの形
		NULL, NULL,						// 背景なし、メニューなし
		clsName,						// クラス名の指定
		NULL							// 小アイコン（なし）
	};

	// ウィンドウクラスの登録
	if (RegisterClassEx(&wcex) == 0) {
		MessageBox(0, L"ウィンドウクラスの登録に失敗しました", NULL, MB_OK);
		return 0;	// 登録失敗
	}

	// ウィンドウの作成
	std::wstring wtitle;
	Poco::UnicodeConverter::toUTF16(_conf.title, wtitle);
	if (_conf.fullsceen) {
		// フルスクリーン
		// 画面全体の幅と高さを取得
		int sw = GetSystemMetrics(SM_CXSCREEN);
		int sh = GetSystemMetrics(SM_CYSCREEN);

		hWnd = CreateWindow(
					wcex.lpszClassName,			// 登録されているクラス名
					wtitle.c_str(), 			// ウインドウ名
					WS_POPUP,					// ウインドウスタイル（ポップアップウインドウを作成）
					0, 							// ウインドウの横方向の位置
					0, 							// ウインドウの縦方向の位置
					_conf.mainRect.right,		// ウインドウの幅
					_conf.mainRect.bottom,		// ウインドウの高さ
					NULL,						// 親ウインドウのハンドル（省略）
					NULL,						// メニューや子ウインドウのハンドル
					hInstance, 					// アプリケーションインスタンスのハンドル
					NULL						// ウインドウの作成データ
				);

	} else {
		// ウィンドウモード
		DWORD dwStyle;
		if (_conf.frame) {
			dwStyle = WS_OVERLAPPEDWINDOW;
		} else {
			dwStyle = WS_POPUP;
		}
		hWnd = CreateWindow(clsName,
							wtitle.c_str(),
							dwStyle,
							CW_USEDEFAULT, CW_USEDEFAULT, 
							CW_USEDEFAULT, CW_USEDEFAULT,
							NULL, NULL, hInstance, NULL);

		if (hWnd) {
			// ウィンドウサイズを再設定する
			RECT rect;
			int ww, wh;
			int cw, ch;

			// ウインドウ全体の横幅の幅を計算
			GetWindowRect(hWnd, &rect);		// ウインドウ全体のサイズ取得
			ww = rect.right - rect.left;	// ウインドウ全体の幅の横幅を計算
			wh = rect.bottom - rect.top;	// ウインドウ全体の幅の縦幅を計算
			
			// クライアント領域の外の幅を計算
			GetClientRect(hWnd, &rect);		// クライアント部分のサイズの取得
			cw = rect.right - rect.left;	// クライアント領域外の横幅を計算
			ch = rect.bottom - rect.top;	// クライアント領域外の縦幅を計算

			ww = ww - cw;					// クライアント領域以外に必要な幅
			wh = wh - ch;					// クライアント領域以外に必要な高さ

			// ウィンドウサイズの再計算
			ww = _conf.mainRect.right + ww;	// 必要なウインドウの幅
			wh = _conf.mainRect.bottom + wh;	// 必要なウインドウの高さ

			// ウインドウサイズの再設定
			SetWindowPos(hWnd, HWND_TOP, _conf.mainRect.left, _conf.mainRect.top, ww, wh, 0);

			// ドラック＆ドロップの受付
			DragAcceptFiles(hWnd, TRUE);
		}
	}
	if (!hWnd) {
		MessageBox(0, L"ウィンドウの生成に失敗しました", NULL, MB_OK);
		return 0;
	}

	// ウィンドウの表示
	UpdateWindow(hWnd);
    ShowWindow(hWnd, nCmdShow);

	// WM_PAINTが呼ばれないようにする
	ValidateRect(hWnd, 0);

#ifdef _DEBUG
	_log.information("*** system start (debug)");
#else 
	#pragma omp parallel
	{
		_log.information(Poco::format("*** system start (omp threads x%d)", omp_get_num_threads()));
	}
#endif

	// レンダラーの初期化
	_renderer = new Renderer();	
	HRESULT hr = _renderer->initialize(hInstance, hWnd);
	if (FAILED(hr)) {
		MessageBox(0, L"レンダラーの初期化に失敗しました", NULL, MB_OK);
		return 0;	// 初期化失敗
	}

	_uim = new ui::UserInterfaceManager(*_renderer);
	_uim->initialize();
	// シーンの生成
	CaptureScenePtr captureScene = NULL;
	if (_conf.useScenes.find("capture") != string::npos) {
		captureScene = new CaptureScene(*_renderer, _uim);
		captureScene->initialize();
		_renderer->addScene("capture", captureScene);
	}
//	WorkspacePtr workspace = new Workspace(_conf.workspaceFile);
//	workspace->parse();
	MainScenePtr mainScene = NULL;
	if (true) {
		mainScene = new MainScene(*_renderer, *_uim, _conf.workspaceFile);
		_renderer->addScene("main", mainScene);
	}
//	UserInterfaceScenePtr uiScene = new UserInterfaceScene(*_renderer, _uim);
//	_renderer->addScene("ui", uiScene);

	Poco::ThreadPool::defaultPool().addCapacity(8);
	Poco::Net::HTTPServerParams* params = new Poco::Net::HTTPServerParams;
	params->setMaxQueued(50);
	params->setMaxThreads(8);
	Poco::Net::ServerSocket socket(9090);
	Poco::Net::HTTPServer* server = new Poco::Net::HTTPServer(new SwitchRequestHandlerFactory(*_renderer), socket, params);
	server->start();

	// メッセージ処理および描画ループ
	EmptyWorkingSet(GetCurrentProcess());
	LARGE_INTEGER freq;
	LARGE_INTEGER start;
	LARGE_INTEGER current;
	::QueryPerformanceFrequency(&freq);
	::QueryPerformanceCounter(&start);

//	DWORD lastSwapout = 0;
	for (;;) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				// PostQuitMessage()が呼ばれた
				break;	//ループの終了
			} else {
				// メッセージの翻訳とディスパッチ
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

		} else {
			// 処理するメッセージが無いときは描画を行う
			if (_interruptFile.length() > 0) {
//				scene->prepareInterruptFile(_interruptFile);
				_interruptFile.clear();
			}

			// ウィンドウが見えている時だけ描画するための処理
			WINDOWPLACEMENT wndpl;
			GetWindowPlacement(hWnd, &wndpl);	// ウインドウの状態を取得
			if ((wndpl.showCmd != SW_HIDE) && 
				(wndpl.showCmd != SW_MINIMIZE) &&
				(wndpl.showCmd != SW_SHOWMINIMIZED) &&
				(wndpl.showCmd != SW_SHOWMINNOACTIVE)) {

				// 描画処理の実行
				::QueryPerformanceCounter(&current);
				DWORD time = (DWORD)((current.QuadPart - start.QuadPart) * 1000 / freq.QuadPart);
				_renderer->renderScene(time);
//				if (lastSwapout == 0 || time - lastSwapout > 3600000) {
//					swapout();
//					lastSwapout = time;
//				}
			}

			timeBeginPeriod(1);
			Sleep(3);
			timeEndPeriod(1);
		}
	}

	_log.information(Poco::format("shutdown web api server: %dthreads", server->currentThreads()));
	server->stop();
	// while (server->currentThreads() > 0) Sleep(200);
	Sleep(1000);
	SAFE_DELETE(server);

	_log.information("shutdown system");
	SAFE_DELETE(_renderer);
//	SAFE_DELETE(workspace);
	SAFE_DELETE(_uim);
	_logFile->release();
//	_log.shutdown();
	CoUninitialize();

	return (int)msg.wParam;
}


HANDLE openVolume(string driveLetter) {
	UINT driveType = GetDriveTypeA(Poco::format("%s:\\", driveLetter).c_str());
	DWORD accessFlags;
	switch (driveType) {
	case DRIVE_REMOVABLE:
	case DRIVE_FIXED: // USB-HDDはこれになる？
		accessFlags = GENERIC_READ | GENERIC_WRITE;
		break;
	case DRIVE_CDROM:
		accessFlags = GENERIC_READ;
		break;
	default:
		_log.warning(Poco::format("cannot eject.  Drive type is incorrect: %s=%?d", driveLetter, driveType));
		return INVALID_HANDLE_VALUE;
	}

	HANDLE volume = CreateFileA(Poco::format("\\\\.\\%s:", driveLetter).c_str(), accessFlags, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (volume == INVALID_HANDLE_VALUE) {
		_log.warning(Poco::format("failed open handle: %s", driveLetter));
	}
	return volume;
}

BOOL closeVolume(HANDLE volume) {
	return CloseHandle(volume);
}

#define LOCK_TIMEOUT        10000       // 10 Seconds
#define LOCK_RETRIES        20

BOOL lockVolume(HANDLE volume) {
	DWORD retBytes;
	DWORD sleepAmount = LOCK_TIMEOUT / LOCK_RETRIES;
	// Do this in a loop until a timeout period has expired
	for (int nTryCount = 0; nTryCount < LOCK_RETRIES; nTryCount++) {
		if (DeviceIoControl(volume, FSCTL_LOCK_VOLUME, NULL, 0,  NULL, 0, &retBytes, NULL)) {
			return TRUE;
		}
		Sleep(sleepAmount);
	}

	return FALSE;
}

// マウント解除
BOOL dismountVolume(HANDLE volume) {
	DWORD retBytes;
	return DeviceIoControl(volume, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &retBytes, NULL);
}

// メディアの強制排出設定
BOOL preventRemovalOfVolume(HANDLE volume, BOOL preventRemoval) {
	DWORD retBytes;
	PREVENT_MEDIA_REMOVAL pmr;
	pmr.PreventMediaRemoval = preventRemoval;
	return DeviceIoControl(volume, IOCTL_STORAGE_MEDIA_REMOVAL, &pmr, sizeof(PREVENT_MEDIA_REMOVAL), NULL, 0, &retBytes,  NULL);
}

// メディアの排出
BOOL autoEjectVolume(HANDLE volume) {
	DWORD retBytes;
	return DeviceIoControl(volume, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &retBytes, NULL);
}

BOOL ejectVolume(string driveLetter) {
	// Open the volume.
	HANDLE volume = openVolume(driveLetter);
	if (volume == INVALID_HANDLE_VALUE)
	return FALSE;

	BOOL removeSafely = FALSE;
	BOOL autoEject = FALSE;
	// Lock and dismount the volume.
	if (lockVolume(volume) && dismountVolume(volume)) {
		removeSafely = TRUE;

		// Set prevent removal to false and eject the volume.
		if (preventRemovalOfVolume(volume, FALSE) && autoEjectVolume(volume)) autoEject = TRUE;
	}
	if (!closeVolume(volume)) return FALSE;

	if (autoEject) {
		_log.warning(Poco::format("media in drive %s has been ejected safely.", driveLetter));
	} else if (removeSafely) {
		_log.information(Poco::format("media in drive %s can be safely removed.", driveLetter));
	}

	return TRUE;
}

//
// この関数は以下のURLよりコピー＆改変
// http://support.microsoft.com/kb/163503/ja
//
string firstDriveFromMask(ULONG unitmask) {
//	_log.information(Poco::format("firstDriveFromMask: %lu", unitmask));
	CHAR i;
	for (i = 0; i < 26; ++i)
	{
		if (unitmask & 0x1) break;
		unitmask = unitmask >> 1;
	}
	vector<CHAR> s;
	s.push_back(i + 'A');
	s.push_back('\0');
//	_log.information(Poco::format("drive: %s", string(&s[0])));
	return &s[0];
}


//-------------------------------------------------------------
// メッセージ処理用コールバック関数
// 引数
//		hWnd	: ウィンドウハンドル
//		msg		: メッセージ
//		wParam	: メッセージの最初のパラメータ
//		lParam	: メッセージの2番目のパラメータ
// 戻り値
//		メッセージ処理結果
//-------------------------------------------------------------
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
		case WM_CREATE:
			::ShowCursor(FALSE);
			break;

		case WM_CLOSE:					// ウインドウが閉じられた
			::ShowCursor(TRUE);
			PostQuitMessage(0);			// アプリケーションを終了する
			break;
		case WM_SETFOCUS:
			break;
		case WM_KILLFOCUS:
			break;

		case WM_IME_SETCONTEXT:
			lParam &= ~ISC_SHOWUIALL;
			break;
		case WM_IME_STARTCOMPOSITION:
		case WM_IME_COMPOSITION:
		case WM_IME_ENDCOMPOSITION:
			return 0;
		case WM_IME_NOTIFY:
			switch(wParam){
			case IMN_OPENSTATUSWINDOW:
			case IMN_CLOSESTATUSWINDOW:
			case IMN_OPENCANDIDATE:
			case IMN_CHANGECANDIDATE:
			case IMN_CLOSECANDIDATE:
				return 0;
			default:
				return DefWindowProc(hWnd, msg, wParam, lParam);
			}

		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE) {
				PostQuitMessage(0);
			} else {
				bool shift = GetKeyState(VK_SHIFT) < 0;
				bool ctrl = GetKeyState(VK_CONTROL) < 0;
				if (_renderer) _renderer->notifyKeyDown(wParam, shift, ctrl);
			}
			break;
		case WM_KEYUP:
			{
				bool shift = GetKeyState(VK_SHIFT) < 0;
				bool ctrl = GetKeyState(VK_CONTROL) < 0;
				if (ctrl && wParam == 'S') {
					swapout();
				} else {
					if (_renderer) _renderer->notifyKeyUp(wParam, shift, ctrl);
				}
			}
			break;

		case WM_MOUSEMOVE:
			if (_uim) _uim->notifyMouseMove(LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_MOUSEWHEEL:
			if (_uim) _uim->notifyMouseWheel((SHORT)HIWORD(wParam));
			break;
		case WM_LBUTTONDOWN:
			if (_uim) _uim->notifyButtonDownL(LOWORD(lParam), HIWORD(lParam));
			::SetCapture(hWnd);
			break;
		case WM_LBUTTONUP:
			if (_uim) _uim->notifyButtonUpL(LOWORD(lParam), HIWORD(lParam));
			::ReleaseCapture();
			break;
		case WM_RBUTTONDOWN:
			_uim->notifyButtonDownR(LOWORD(lParam), HIWORD(lParam));
			::SetCapture(hWnd);
			break;
		case WM_RBUTTONUP:
			if (_uim) _uim->notifyButtonUpR(LOWORD(lParam), HIWORD(lParam));
			::ReleaseCapture();
			break;

		case WM_DROPFILES:
			{
				HDROP hDrop = (HDROP)wParam; /* HDROPを取得 */
				vector<WCHAR> dropFile(255);
				DragQueryFile(hDrop, 0, &dropFile[0], 256); /* 最初のファイル名を取得 */
				DragFinish(hDrop); /* ドロップの終了処理 */
				Poco::UnicodeConverter::toUTF8(&dropFile[0], _interruptFile);
			}
			break;

		case WM_DEVICECHANGE:
			{
				bool addData = false;
				wstring ws;
				switch (wParam) {
				case DBT_CONFIGCHANGECANCELED:
					ws = L"DBT_CONFIGCHANGECANCELED：設定変更要求がキャンセルされました";
					break;

				case DBT_CONFIGCHANGED:
					ws = L"DBT_CONFIGCHANGED：設定が変更されました";
					break;

				case DBT_CUSTOMEVENT:
					ws = L"DBT_CUSTOMEVENT：ドライバー定義のカスタムイベントが発行されました";
					addData = true;
					break;

				case DBT_DEVICEARRIVAL:
					ws = L"DBT_DEVICEARRIVAL：デバイスが使用可能になりました";
					addData = true;
					{
						DEV_BROADCAST_HDR* data = (DEV_BROADCAST_HDR*)lParam;
						if (data && data->dbch_devicetype == DBT_DEVTYP_VOLUME) {
							DEV_BROADCAST_VOLUME* extend = (DEV_BROADCAST_VOLUME*)data;
							if (extend) {
								//ejectVolume(firstDriveFromMask(extend->dbcv_unitmask));
							}
						}
					}
					break;

				case DBT_DEVICEQUERYREMOVE:
					ws = L"DBT_DEVICEQUERYREMOVE：デバイス停止要求が発行されました";
					addData = true;
					break;

				case DBT_DEVICEQUERYREMOVEFAILED:
					ws = L"DBT_DEVICEQUERYREMOVEFAILED：デバイス停止要求が失敗されました";
					addData = true;
					break;

				case DBT_DEVICEREMOVECOMPLETE:
					ws = L"DBT_DEVICEREMOVECOMPLETE：デバイスが停止されました";
					addData = true;
					break;

				case DBT_DEVICEREMOVEPENDING:
					ws = L"DBT_DEVICEREMOVEPENDING：デバイスを停止中です";
					addData = true;
					break;

				case DBT_DEVICETYPESPECIFIC:
					ws = L"DBT_DEVICETYPESPECIFIC：デバイスの独自イベントが発行されました";
					addData = true;
					break;

				case DBT_DEVNODES_CHANGED:
					ws = L"DBT_DEVNODES_CHANGED：システムのデバイス状態が変化しました";
					break;

				case DBT_QUERYCHANGECONFIG:
					ws = L"DBT_QUERYCHANGECONFIG：設定変更要求が発行されました";
					break;

				case DBT_USERDEFINED:
					ws = L"DBT_USERDEFINED";
					break;
				}

				string p;
				if (addData) {
					DEV_BROADCAST_HDR* data = (DEV_BROADCAST_HDR*)lParam;
					if (data) {
						string type;
						switch (data->dbch_devicetype) {
						case DBT_DEVTYP_OEM:
							{
								type="OEM";
								DEV_BROADCAST_OEM* extend = (DEV_BROADCAST_OEM*)data;
								if (extend) {
								}
							}
							break;

						case DBT_DEVTYP_DEVNODE:
							{
								type="DEVNODE";
								DEV_BROADCAST_DEVNODE* extend = (DEV_BROADCAST_DEVNODE*)data;
								if (extend) {
								}
							}
							break;

						case DBT_DEVTYP_VOLUME:
							{
								type="VOLUME";
								DEV_BROADCAST_VOLUME* extend = (DEV_BROADCAST_VOLUME*)data;
								if (extend) {
									string s = firstDriveFromMask(extend->dbcv_unitmask);
									type = Poco::format("volue[%s:]", s);
								}
							}
							break;

						case DBT_DEVTYP_PORT:
							{
								type="PORT";
								DEV_BROADCAST_PORT* extend = (DEV_BROADCAST_PORT*)data;
								if (extend) {
									string s;
									Poco::UnicodeConverter::toUTF8(extend->dbcp_name, s);
									type = Poco::format("PORT %s", s);
								}
							}
							break;

						case DBT_DEVTYP_NET:
							{
								type="NET";
								DEV_BROADCAST_NET* extend = (DEV_BROADCAST_NET*)data;
								if (extend) {
								}
							}
							break;

						case DBT_DEVTYP_DEVICEINTERFACE:
							{
								type="DEVICEINTERFACE";
								DEV_BROADCAST_DEVICEINTERFACE* extend = (DEV_BROADCAST_DEVICEINTERFACE*)data;
								if (extend) {
									string s;
									Poco::UnicodeConverter::toUTF8(extend->dbcc_name, s);
									type = Poco::format("I/F %s", s);
								}
							}
							break;

						case DBT_DEVTYP_HANDLE:
							{
								type="HANDLE";
								DEV_BROADCAST_HANDLE* extend = (DEV_BROADCAST_HANDLE*)data;
								if (extend && extend->dbch_handle && extend->dbch_handle != INVALID_HANDLE_VALUE) {
								}
							}
							break;
						default:
							type = Poco::format("%08d", data->dbch_devicetype);
						}
						p = Poco::format(" (Size=%d DeviceType=%s) ", ((int)data->dbch_size), type);
					}
				}

				string s;
				Poco::UnicodeConverter::toUTF8(ws, s);
				if (!p.empty()) s.append(p);
				_log.information(Poco::format("device change: %s",s));
				// SetupDiEnumDeviceInfo, SetupDiGetDeviceRegistryProperty, CM_Request_Device_Eject
			}
			break;

		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}


// GUIの設定
bool guiConfiguration()
{
	// ffmpegの初期化
	avcodec_register_all();
	avdevice_register_all();
	av_register_all();

	try {
		XMLConfiguration* xml = new XMLConfiguration("switch-config.xml");
		Poco::PatternFormatter* pat = new Poco::PatternFormatter(xml->getString("log[@pattern]", "%Y-%m-%d %H:%M:%S.%c %N[%T]:%t"));
		pat->setProperty(Poco::PatternFormatter::PROP_TIMES, "local");
		Poco::FormattingChannel* fc = new Poco::FormattingChannel(pat);
//		_logFile = new Poco::ConsoleChannel();
		_logFile = new Poco::FileChannel(xml->getString("log", "switch.log"));
		fc->setChannel(_logFile);
		_log.setChannel(fc);
		// ローカル時刻指定
		fc->setProperty(Poco::FileChannel::PROP_TIMES, "local");
		// アーカイブファイル名への付加文字列[number/timestamp] (日付)
		fc->setProperty(Poco::FileChannel::PROP_ARCHIVE, xml->getString("log[@archive]", "timestamp"));
		// 圧縮[true/false] (あり)
		fc->setProperty(Poco::FileChannel::PROP_COMPRESS, xml->getString("log[@compress]", "true"));
		// ローテーション単位[never/[day,][hh]:mm/daily/weekly/monthly/<n>minutes/hours/days/weeks/months/<n>/<n>K/<n>M] (日)
		fc->setProperty(Poco::FileChannel::PROP_ROTATION, xml->getString("log[@rotation]", "daily"));
		// 保持期間[<n>seconds/<n>minutes/<n>hours/<n>days/<n>weeks/<n>months] (5日間)
		fc->setProperty(Poco::FileChannel::PROP_PURGEAGE, xml->getString("log[@purgeage]", "5days"));
		fc->release();
		pat->release();
		_log.information("*** configuration");

		_conf.title = xml->getString("title", "switch");
		_conf.mainRect.left = xml->getInt("display.x", 0);
		_conf.mainRect.top = xml->getInt("display.y", 0);
		int w = xml->getInt("display.width", 1024);
		int h = xml->getInt("display.height", 768);
		_conf.mainRect.right = w;
		_conf.mainRect.bottom = h;
		_conf.mainRate = xml->getInt("display.rate", D3DPRESENT_RATE_DEFAULT);
		_conf.subRect.left = xml->getInt("display[1].x", _conf.mainRect.left);
		_conf.subRect.top = xml->getInt("display[1].y", _conf.mainRect.top);
		_conf.subRect.right = xml->getInt("display[1].width", _conf.mainRect.right);
		_conf.subRect.bottom = xml->getInt("display[1].height", _conf.mainRect.bottom);
		_conf.subRate = xml->getInt("display[1].rate", _conf.mainRate);
		_conf.frameIntervals = xml->getInt("display.frameIntervals", 3);
		_conf.frame = xml->getBool("display.frame", true);
		_conf.fullsceen = xml->getBool("display.fullscreen", true);
		_conf.draggable = xml->getBool("display.draggable", true);
		_conf.mouse = xml->getBool("display.mouse", true);
		string windowStyles(_conf.fullsceen?"fullscreen":"window");
		_log.information(Poco::format("display %dx%d@%d %s", w, h, _conf.mainRate, windowStyles));
		_conf.useClip = xml->getBool("display.clip.use", false);
		_conf.clipRect.left = xml->getInt("display.clip.x1", 0);
		_conf.clipRect.top = xml->getInt("display.clip.y1", 0);
		_conf.clipRect.right = xml->getInt("display.clip.x2", 0);
		_conf.clipRect.bottom = xml->getInt("display.clip.y2", 0);
		string useClip(_conf.useClip?"use":"not use");
		_log.information(Poco::format("clip [%s] %ld,%ld %ldx%ld", useClip, _conf.clipRect.left, _conf.clipRect.top, _conf.clipRect.right, _conf.clipRect.bottom));

		int cw = xml->getInt("display.split.width", w);
		int ch = xml->getInt("display.split.height", h);
		int cycles = xml->getInt("display.split.cycles", h / ch);
		_conf.splitSize.cx = cw;
		_conf.splitSize.cy = ch;
		_conf.stageRect.left = xml->getInt("stage.x", 0);
		_conf.stageRect.top = xml->getInt("stage.y", 0);
		_conf.stageRect.right = xml->getInt("stage.width", w * cycles);
		_conf.stageRect.bottom = xml->getInt("stage.height", ch);
		_conf.splitCycles = cycles;
		string splitType = xml->getString("display.split.type", "none");
		if (splitType == "vertical" || splitType == "vertical-down") {
			_conf.splitType = 1;
		} else if (splitType == "vertical-up") {
			_conf.splitType = 2;
		} else if (splitType == "horizontal") {
			_conf.splitType = 11;
		} else {
			_conf.splitType = 0;
		}
		_log.information(Poco::format("stage (%ld,%ld) %ldx%ld", _conf.stageRect.left, _conf.stageRect.top, _conf.stageRect.right, _conf.stageRect.bottom));
		_log.information(Poco::format("split <%s:%d> %dx%d x%d", splitType, _conf.splitType, cw, ch, cycles));

		_conf.useScenes = xml->getString("scenes", "main,operation");
		_conf.luminance = xml->getInt("stage.luminance", 100);

		_conf.imageSplitWidth = xml->getInt("stage.imageSplitWidth", 0);
		if (xml->hasProperty("stage.text")) {
			string s;
			Poco::UnicodeConverter::toUTF8(L"ＭＳ ゴシック", s);
			_conf.textFont = xml->getString("stage.text.font", s);
			string style = xml->getString("stage.text.style");
			if (style == "bold") {
				_conf.textStyle = Gdiplus::FontStyleBold;
			} else if (style == "italic") {
				_conf.textStyle = Gdiplus::FontStyleItalic;
			} else if (style == "bolditalic") {
				_conf.textStyle = Gdiplus::FontStyleBoldItalic;
			} else {
				_conf.textStyle = Gdiplus::FontStyleRegular;
			}
			_conf.textHeight = xml->getInt("stage.text.height", _conf.stageRect.bottom - 2);
		} else {
			string s;
			Poco::UnicodeConverter::toUTF8(L"ＭＳ ゴシック", s);
			_conf.textFont = s;
			_conf.textStyle = Gdiplus::FontStyleRegular;
			_conf.textHeight = _conf.stageRect.bottom - 2;
		}

		string defaultFont = xml->getString("ui.defaultFont", "");
		wstring ws;
		Poco::UnicodeConverter::toUTF16(defaultFont, ws);
		_conf.defaultFont = ws;
		_conf.asciiFont = xml->getString("ui.asciiFont", "Defactica");
		_conf.multiByteFont = xml->getString("ui.multiByteFont", "A-OTF-ShinGoPro-Regular.ttf");
//		_conf.vpCommandFile = xml->getString("vpCommand", "");
//		_conf.monitorFile = xml->getString("monitor", "");
		_conf.dataRoot = Path(xml->getString("data-root", "")).absolute();
		_log.information(Poco::format("data root: %s", _conf.dataRoot.toString()));
		_conf.workspaceFile = Path(_conf.dataRoot, xml->getString("workspace", "workspace.xml"));
		_log.information(Poco::format("workspace: %s", _conf.workspaceFile.toString()));
		_conf.newsURL = xml->getString("newsURL", "https://led.avix.co.jp:8080/news");
		xml->release();
		return true;
	} catch (Poco::Exception& ex) {
		string s;
		Poco::UnicodeConverter::toUTF8(L"設定ファイル(switch-config.xml)を確認してください\n「%s」", s);
		wstring utf16;
		Poco::UnicodeConverter::toUTF16(Poco::format(s, ex.displayText()), utf16);
		::MessageBox(HWND_DESKTOP, utf16.c_str(), L"エラー", MB_OK);
		_log.warning(ex.displayText());
	}
	return false;
}

Configuration& config() {
	return _conf;
}

// スワップアウト
void swapout() {
	_log.information("*** exec memory swapout");
	EmptyWorkingSet(GetCurrentProcess());
	return;

	DWORD idProcess[1024];
	DWORD bsize = 0;
	/* プロセス識別子を取得する */
	if (EnumProcesses(idProcess, sizeof(idProcess), &bsize) == FALSE) {
		_log.warning("failed EnumProcesses()");
		return;
	}
	_log.information("*** exec memory swapout");
	int proc_num = bsize / sizeof(DWORD);
	WCHAR name[1024];
	for (int i = 0; i < proc_num; i++) {
		/* プロセスハンドルを取得する */
		HANDLE	handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_SET_QUOTA | PROCESS_VM_READ, FALSE, idProcess[i]);
		if (handle != NULL) {
			bool swapouted = false;
			if (handle != GetCurrentProcess()) {
				// 自分以外のプロセスをスワップアウトする
				swapouted = EmptyWorkingSet(handle)==TRUE;
			}
			HMODULE	module[1024];
			if (EnumProcessModules(handle, module, sizeof(module), &bsize) != FALSE ) {
				/* プロセス名を取得する */
				if (GetModuleBaseName(handle, module[0], name, sizeof(name)) > 0) {
					PROCESS_MEMORY_COUNTERS pmc = {0};
					GetProcessMemoryInfo(handle, &pmc, sizeof(PROCESS_MEMORY_COUNTERS));
					int mem = pmc.WorkingSetSize / 1024;
					string swapout = swapouted?"<swapouted>":"";
					string nameUTF8;
					Poco::UnicodeConverter::toUTF8(wstring(name), nameUTF8);
					_log.information(Poco::format("process: %20s %05dkB%s", nameUTF8, mem, swapout));
				}
			}
			CloseHandle(handle);
		}
	}
}
