//
// switch.cpp
// アプリケーションの実装
//

#include <winsock2.h>
#include <windows.h>
#include <psapi.h>
#include <Dbt.h>

#include <Poco/format.h>
#include <Poco/Logger.h>
#include <Poco/UnicodeConverter.h>
#include "Poco/Net/HTTPStreamFactory.h"
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/ServerSocket.h>

#include "switch.h"
#include "Renderer.h"
#include "CaptureScene.h"
#include "MainScene.h"
#include "DiffDetectScene.h"
//#include "UserInterfaceScene.h"
#include "Utils.h"
#include "WebAPI.h"
//#include "ui/UserInterfaceManager.h"

//#ifndef _DEBUG
//#include <omp.h>
//#endif

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>                // 最後にインクルードしたほうがいい気がする。C++の場合。標準ヘッダの中にnewとかあると変になる？？
#ifdef _DEBUG
#define new ::new(_NORMAL_BLOCK,__FILE__,__LINE__)     // これが重要
#endif


static TCHAR clsName[] = TEXT("switchClass"); // クラス名

static Configuration _conf;

static RendererPtr _renderer;
//static ui::UserInterfaceManagerPtr _uim;

//static string _interruptFile;


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
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
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

	_conf.initialize();
	Poco::Logger& log = Poco::Logger::get("");
#ifdef _DEBUG
	log.information("*** system start (debug)");
#else 
	//#pragma omp parallel
	//{
	//	log.information(Poco::format("*** system start (omp threads x%d)", omp_get_num_threads()));
	//}
#endif
	vector<string> args;
	svvitch::split(lpCmdLine, ' ', args);
	for (vector<string>::iterator it = args.begin(); it != args.end(); it++) {
		string opt = Poco::toLower(*it);
		if (opt == "-w") {
			it++;
			if (it != args.end()) {
				long t = Poco::NumberParser::parse(*it);
				log.information(Poco::format("startup waiting: %lds", t));
				Poco::Thread::sleep(t * 1000);
			}
		}
	}

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
	Poco::UnicodeConverter::toUTF16(_conf.windowTitle, wtitle);
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

	// レンダラーの初期化
	_renderer = new Renderer();	
	HRESULT hr = _renderer->initialize(hInstance, hWnd);
	if (FAILED(hr)) {
		MessageBox(0, L"レンダラーの初期化に失敗しました", NULL, MB_OK);
		return 0;	// 初期化失敗
	}

	//_uim = new ui::UserInterfaceManager(*_renderer);
	//_uim->initialize();
	// シーンの生成
	CaptureScenePtr captureScene = NULL;
	if (_conf.hasScene("capture")) {
		captureScene = new CaptureScene(*_renderer);
		captureScene->initialize();
		_renderer->addScene("capture", captureScene);
	}
	MainScenePtr mainScene = NULL;
	if (true) {
		mainScene = new MainScene(*_renderer);
		_renderer->addScene("main", mainScene);
	}
	//if (_conf.hasScene("diff")) {
	//	DiffDetectScenePtr diffScene = new DiffDetectScene(*_renderer);
	//	diffScene->initialize();
	//	_renderer->addScene("diff", diffScene);
	//}
	// UserInterfaceScenePtr uiScene = new UserInterfaceScene(*_renderer, _uim);
	// _renderer->addScene("ui", uiScene);

	Poco::Net::HTTPStreamFactory::registerFactory();
	Poco::ThreadPool::defaultPool().addCapacity(8);
	Poco::Net::HTTPServerParams* params = new Poco::Net::HTTPServerParams;
	params->setMaxQueued(_conf.maxQueued);
	params->setMaxThreads(_conf.maxThreads);
	Poco::Net::ServerSocket socket(_conf.serverPort);
	Poco::Net::HTTPServer* server = new Poco::Net::HTTPServer(new SwitchRequestHandlerFactory(*_renderer), socket, params);
	server->start();

	// メッセージ処理および描画ループ
	//EmptyWorkingSet(GetCurrentProcess());
	//::SetThreadAffinityMask(::GetCurrentThread(), 1);
	LARGE_INTEGER freq;
	LARGE_INTEGER start;
	LARGE_INTEGER current;
	::QueryPerformanceFrequency(&freq);
	::QueryPerformanceCounter(&start);

//	DWORD lastSwapout = 0;
	DWORD last = 0;
	while (_renderer->peekMessage()) {
		// 処理するメッセージが無いときは描画を行う
		//if (_interruptFile.length() > 0) {
		//	scene->prepareInterruptFile(_interruptFile);
		//	_interruptFile.clear();
		//}

		// ウィンドウが見えている時だけ描画するための処理
		WINDOWPLACEMENT wndpl;
		GetWindowPlacement(hWnd, &wndpl);	// ウインドウの状態を取得
		if ((wndpl.showCmd != SW_HIDE) && 
			(wndpl.showCmd != SW_MINIMIZE) &&
			(wndpl.showCmd != SW_SHOWMINIMIZED) &&
			(wndpl.showCmd != SW_SHOWMINNOACTIVE)) {

			::QueryPerformanceCounter(&current);
			DWORD time = (DWORD)((current.QuadPart - start.QuadPart) * 1000 / freq.QuadPart);
			last = time;

			// 描画処理の実行
			_renderer->renderScene(time);
			// if (lastSwapout == 0 || time - lastSwapout > 3600000) {
			//	swapout();
			//	lastSwapout = time;
			// }
		}
		Poco::Thread::sleep(_conf.frameIntervals);
	}

	log.information(Poco::format("shutdown web api server: %dthreads", server->currentThreads()));
	server->stop();
	DWORD time = last;
	while (time - last < 500) {
		_renderer->peekMessage();
		::QueryPerformanceCounter(&current);
		time = (DWORD)((current.QuadPart - start.QuadPart) * 1000 / freq.QuadPart);
	}
	SAFE_DELETE(server);

	int exitCode = _renderer->getExitCode();
	log.information(Poco::format("shutdown system (%d)", exitCode));
	SAFE_DELETE(_renderer);
	//SAFE_DELETE(_uim);
	_conf.save();
	_conf.release();
	CoUninitialize();

	UnregisterClass(clsName, wcex.hInstance);
	return exitCode;
}

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
			//if (_uim) _uim->notifyMouseMove(LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_MOUSEWHEEL:
			//if (_uim) _uim->notifyMouseWheel((SHORT)HIWORD(wParam));
			break;
		case WM_LBUTTONDOWN:
			//if (_uim) _uim->notifyButtonDownL(LOWORD(lParam), HIWORD(lParam));
			::SetCapture(hWnd);
			break;
		case WM_LBUTTONUP:
			//if (_uim) _uim->notifyButtonUpL(LOWORD(lParam), HIWORD(lParam));
			::ReleaseCapture();
			break;
		case WM_RBUTTONDOWN:
			//_uim->notifyButtonDownR(LOWORD(lParam), HIWORD(lParam));
			::SetCapture(hWnd);
			break;
		case WM_RBUTTONUP:
			//if (_uim) _uim->notifyButtonUpR(LOWORD(lParam), HIWORD(lParam));
			::ReleaseCapture();
			break;

		case WM_DROPFILES:
			{
				HDROP hDrop = (HDROP)wParam; /* HDROPを取得 */
				vector<WCHAR> dropFile(255);
				DragQueryFile(hDrop, 0, &dropFile[0], 256); /* 最初のファイル名を取得 */
				DragFinish(hDrop); /* ドロップの終了処理 */
				//Poco::UnicodeConverter::toUTF8(&dropFile[0], _interruptFile);
			}
			break;

		case WM_DEVICECHANGE:
			{
				switch (wParam) {
				case DBT_DEVICEARRIVAL:
					{
						DEV_BROADCAST_HDR* data = (DEV_BROADCAST_HDR*)lParam;
						if (data && data->dbch_devicetype == DBT_DEVTYP_VOLUME) {
							DEV_BROADCAST_VOLUME* extend = (DEV_BROADCAST_VOLUME*)data;
							if (extend && _renderer) _renderer->addDrive(extend->dbcv_unitmask);
						}
					}
					break;
				case DBT_DEVICEREMOVECOMPLETE:
					{
						DEV_BROADCAST_HDR* data = (DEV_BROADCAST_HDR*)lParam;
						if (data && data->dbch_devicetype == DBT_DEVTYP_VOLUME) {
							DEV_BROADCAST_VOLUME* extend = (DEV_BROADCAST_VOLUME*)data;
							if (extend && _renderer) _renderer->removeDrive(extend->dbcv_unitmask);
						}
					}
					break;
				case DBT_DEVNODES_CHANGED:
					break;
				}
				_renderer->deviceChanged();
			}
			break;

		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}


Configuration& config() {
	return _conf;
}

void swapout() {
	Poco::Logger& log = Poco::Logger::get("");
	log.information("*** exec memory swapout");
	EmptyWorkingSet(GetCurrentProcess());
	return;

	DWORD idProcess[1024];
	DWORD bsize = 0;
	/* プロセス識別子を取得する */
	if (EnumProcesses(idProcess, sizeof(idProcess), &bsize) == FALSE) {
		log.warning("failed EnumProcesses()");
		return;
	}
	log.information("*** exec memory swapout");
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
					log.information(Poco::format("process: %20s %05dkB%s", nameUTF8, mem, swapout));
				}
			}
			CloseHandle(handle);
		}
	}
}
