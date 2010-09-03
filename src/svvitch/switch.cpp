//
// switch.cpp
// �A�v���P�[�V�����̎���
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
#include <crtdbg.h>                // �Ō�ɃC���N���[�h�����ق��������C������BC++�̏ꍇ�B�W���w�b�_�̒���new�Ƃ�����ƕςɂȂ�H�H
#ifdef _DEBUG
#define new ::new(_NORMAL_BLOCK,__FILE__,__LINE__)     // ���ꂪ�d�v
#endif


static TCHAR clsName[] = TEXT("switchClass"); // �N���X��

static Configuration _conf;

static RendererPtr _renderer;
//static ui::UserInterfaceManagerPtr _uim;

//static string _interruptFile;


//-------------------------------------------------------------
// �A�v���P�[�V�����̃G���g���|�C���g
// ����
//		hInstance     : ���݂̃C���X�^���X�̃n���h��
//		hPrevInstance : �ȑO�̃C���X�^���X�̃n���h��
//		lpCmdLine	  : �R�}���h���C���p�����[�^
//		nCmdShow	  : �E�B���h�E�̕\�����
// �߂�l
//		����������0�ȊO�̒l
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
	// �E�B���h�E�N���X�̏�����
	WNDCLASSEX wcex = {
		sizeof(WNDCLASSEX),				// ���̍\���̂̃T�C�Y
		CS_DBLCLKS,						// �E�C���h�E�̃X�^�C��(default)
		WindowProc,						// ���b�Z�[�W�����֐��̓o�^
		0,								// �ʏ�͎g��Ȃ��̂ŏ��0
		0,								// �ʏ�͎g��Ȃ��̂ŏ��0
		hInstance,						// �C���X�^���X�ւ̃n���h��
		NULL,							// �A�C�R���i�Ȃ��j
		LoadCursor(NULL, IDC_ARROW),	// �J�[�\���̌`
		NULL, NULL,						// �w�i�Ȃ��A���j���[�Ȃ�
		clsName,						// �N���X���̎w��
		NULL							// ���A�C�R���i�Ȃ��j
	};

	// �E�B���h�E�N���X�̓o�^
	if (RegisterClassEx(&wcex) == 0) {
		MessageBox(0, L"�E�B���h�E�N���X�̓o�^�Ɏ��s���܂���", NULL, MB_OK);
		return 0;	// �o�^���s
	}

	// �E�B���h�E�̍쐬
	std::wstring wtitle;
	Poco::UnicodeConverter::toUTF16(_conf.windowTitle, wtitle);
	if (_conf.fullsceen) {
		// �t���X�N���[��
		// ��ʑS�̂̕��ƍ������擾
		int sw = GetSystemMetrics(SM_CXSCREEN);
		int sh = GetSystemMetrics(SM_CYSCREEN);

		hWnd = CreateWindow(
					wcex.lpszClassName,			// �o�^����Ă���N���X��
					wtitle.c_str(), 			// �E�C���h�E��
					WS_POPUP,					// �E�C���h�E�X�^�C���i�|�b�v�A�b�v�E�C���h�E���쐬�j
					0, 							// �E�C���h�E�̉������̈ʒu
					0, 							// �E�C���h�E�̏c�����̈ʒu
					_conf.mainRect.right,		// �E�C���h�E�̕�
					_conf.mainRect.bottom,		// �E�C���h�E�̍���
					NULL,						// �e�E�C���h�E�̃n���h���i�ȗ��j
					NULL,						// ���j���[��q�E�C���h�E�̃n���h��
					hInstance, 					// �A�v���P�[�V�����C���X�^���X�̃n���h��
					NULL						// �E�C���h�E�̍쐬�f�[�^
				);

	} else {
		// �E�B���h�E���[�h
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
			// �E�B���h�E�T�C�Y���Đݒ肷��
			RECT rect;
			int ww, wh;
			int cw, ch;

			// �E�C���h�E�S�̂̉����̕����v�Z
			GetWindowRect(hWnd, &rect);		// �E�C���h�E�S�̂̃T�C�Y�擾
			ww = rect.right - rect.left;	// �E�C���h�E�S�̂̕��̉������v�Z
			wh = rect.bottom - rect.top;	// �E�C���h�E�S�̂̕��̏c�����v�Z
			
			// �N���C�A���g�̈�̊O�̕����v�Z
			GetClientRect(hWnd, &rect);		// �N���C�A���g�����̃T�C�Y�̎擾
			cw = rect.right - rect.left;	// �N���C�A���g�̈�O�̉������v�Z
			ch = rect.bottom - rect.top;	// �N���C�A���g�̈�O�̏c�����v�Z

			ww = ww - cw;					// �N���C�A���g�̈�ȊO�ɕK�v�ȕ�
			wh = wh - ch;					// �N���C�A���g�̈�ȊO�ɕK�v�ȍ���

			// �E�B���h�E�T�C�Y�̍Čv�Z
			ww = _conf.mainRect.right + ww;	// �K�v�ȃE�C���h�E�̕�
			wh = _conf.mainRect.bottom + wh;	// �K�v�ȃE�C���h�E�̍���

			// �E�C���h�E�T�C�Y�̍Đݒ�
			SetWindowPos(hWnd, HWND_TOP, _conf.mainRect.left, _conf.mainRect.top, ww, wh, 0);

			// �h���b�N���h���b�v�̎�t
			DragAcceptFiles(hWnd, TRUE);
		}
	}
	if (!hWnd) {
		MessageBox(0, L"�E�B���h�E�̐����Ɏ��s���܂���", NULL, MB_OK);
		return 0;
	}

	// �E�B���h�E�̕\��
	UpdateWindow(hWnd);
    ShowWindow(hWnd, nCmdShow);

	// WM_PAINT���Ă΂�Ȃ��悤�ɂ���
	ValidateRect(hWnd, 0);

	// �����_���[�̏�����
	_renderer = new Renderer();	
	HRESULT hr = _renderer->initialize(hInstance, hWnd);
	if (FAILED(hr)) {
		MessageBox(0, L"�����_���[�̏������Ɏ��s���܂���", NULL, MB_OK);
		return 0;	// ���������s
	}

	//_uim = new ui::UserInterfaceManager(*_renderer);
	//_uim->initialize();
	// �V�[���̐���
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

	// ���b�Z�[�W��������ѕ`�惋�[�v
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
		// �������郁�b�Z�[�W�������Ƃ��͕`����s��
		//if (_interruptFile.length() > 0) {
		//	scene->prepareInterruptFile(_interruptFile);
		//	_interruptFile.clear();
		//}

		// �E�B���h�E�������Ă��鎞�����`�悷�邽�߂̏���
		WINDOWPLACEMENT wndpl;
		GetWindowPlacement(hWnd, &wndpl);	// �E�C���h�E�̏�Ԃ��擾
		if ((wndpl.showCmd != SW_HIDE) && 
			(wndpl.showCmd != SW_MINIMIZE) &&
			(wndpl.showCmd != SW_SHOWMINIMIZED) &&
			(wndpl.showCmd != SW_SHOWMINNOACTIVE)) {

			::QueryPerformanceCounter(&current);
			DWORD time = (DWORD)((current.QuadPart - start.QuadPart) * 1000 / freq.QuadPart);
			last = time;

			// �`�揈���̎��s
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

		case WM_CLOSE:					// �E�C���h�E������ꂽ
			::ShowCursor(TRUE);
			PostQuitMessage(0);			// �A�v���P�[�V�������I������
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
				HDROP hDrop = (HDROP)wParam; /* HDROP���擾 */
				vector<WCHAR> dropFile(255);
				DragQueryFile(hDrop, 0, &dropFile[0], 256); /* �ŏ��̃t�@�C�������擾 */
				DragFinish(hDrop); /* �h���b�v�̏I������ */
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
	/* �v���Z�X���ʎq���擾���� */
	if (EnumProcesses(idProcess, sizeof(idProcess), &bsize) == FALSE) {
		log.warning("failed EnumProcesses()");
		return;
	}
	log.information("*** exec memory swapout");
	int proc_num = bsize / sizeof(DWORD);
	WCHAR name[1024];
	for (int i = 0; i < proc_num; i++) {
		/* �v���Z�X�n���h�����擾���� */
		HANDLE	handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_SET_QUOTA | PROCESS_VM_READ, FALSE, idProcess[i]);
		if (handle != NULL) {
			bool swapouted = false;
			if (handle != GetCurrentProcess()) {
				// �����ȊO�̃v���Z�X���X���b�v�A�E�g����
				swapouted = EmptyWorkingSet(handle)==TRUE;
			}
			HMODULE	module[1024];
			if (EnumProcessModules(handle, module, sizeof(module), &bsize) != FALSE ) {
				/* �v���Z�X�����擾���� */
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
