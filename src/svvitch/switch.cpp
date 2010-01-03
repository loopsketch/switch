//
// switch.cpp
// �A�v���P�[�V�����̎���
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
#include <crtdbg.h>                // �Ō�ɃC���N���[�h�����ق��������C������BC++�̏ꍇ�B�W���w�b�_�̒���new�Ƃ�����ƕςɂȂ�H�H
#ifdef _DEBUG
#define new ::new(_NORMAL_BLOCK,__FILE__,__LINE__)     // ���ꂪ�d�v
#endif

using Poco::AutoPtr;
using Poco::File;
using Poco::Util::XMLConfiguration;
using Poco::XML::Document;
using Poco::XML::Element;


static TCHAR clsName[] = TEXT("switchClass"); // �N���X��

static Poco::Channel* _logFile;
static Poco::Logger& _log = Poco::Logger::get("");
static Configuration _conf;

static RendererPtr _renderer;
static ui::UserInterfaceManagerPtr _uim;

static std::string _interruptFile;


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
	Poco::UnicodeConverter::toUTF16(_conf.title, wtitle);
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

#ifdef _DEBUG
	_log.information("*** system start (debug)");
#else 
	#pragma omp parallel
	{
		_log.information(Poco::format("*** system start (omp threads x%d)", omp_get_num_threads()));
	}
#endif

	// �����_���[�̏�����
	_renderer = new Renderer();	
	HRESULT hr = _renderer->initialize(hInstance, hWnd);
	if (FAILED(hr)) {
		MessageBox(0, L"�����_���[�̏������Ɏ��s���܂���", NULL, MB_OK);
		return 0;	// ���������s
	}

	_uim = new ui::UserInterfaceManager(*_renderer);
	_uim->initialize();
	// �V�[���̐���
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

	// ���b�Z�[�W��������ѕ`�惋�[�v
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
				// PostQuitMessage()���Ă΂ꂽ
				break;	//���[�v�̏I��
			} else {
				// ���b�Z�[�W�̖|��ƃf�B�X�p�b�`
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

		} else {
			// �������郁�b�Z�[�W�������Ƃ��͕`����s��
			if (_interruptFile.length() > 0) {
//				scene->prepareInterruptFile(_interruptFile);
				_interruptFile.clear();
			}

			// �E�B���h�E�������Ă��鎞�����`�悷�邽�߂̏���
			WINDOWPLACEMENT wndpl;
			GetWindowPlacement(hWnd, &wndpl);	// �E�C���h�E�̏�Ԃ��擾
			if ((wndpl.showCmd != SW_HIDE) && 
				(wndpl.showCmd != SW_MINIMIZE) &&
				(wndpl.showCmd != SW_SHOWMINIMIZED) &&
				(wndpl.showCmd != SW_SHOWMINNOACTIVE)) {

				// �`�揈���̎��s
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
	case DRIVE_FIXED: // USB-HDD�͂���ɂȂ�H
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

// �}�E���g����
BOOL dismountVolume(HANDLE volume) {
	DWORD retBytes;
	return DeviceIoControl(volume, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &retBytes, NULL);
}

// ���f�B�A�̋����r�o�ݒ�
BOOL preventRemovalOfVolume(HANDLE volume, BOOL preventRemoval) {
	DWORD retBytes;
	PREVENT_MEDIA_REMOVAL pmr;
	pmr.PreventMediaRemoval = preventRemoval;
	return DeviceIoControl(volume, IOCTL_STORAGE_MEDIA_REMOVAL, &pmr, sizeof(PREVENT_MEDIA_REMOVAL), NULL, 0, &retBytes,  NULL);
}

// ���f�B�A�̔r�o
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
// ���̊֐��͈ȉ���URL���R�s�[������
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
// ���b�Z�[�W�����p�R�[���o�b�N�֐�
// ����
//		hWnd	: �E�B���h�E�n���h��
//		msg		: ���b�Z�[�W
//		wParam	: ���b�Z�[�W�̍ŏ��̃p�����[�^
//		lParam	: ���b�Z�[�W��2�Ԗڂ̃p�����[�^
// �߂�l
//		���b�Z�[�W��������
//-------------------------------------------------------------
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
				HDROP hDrop = (HDROP)wParam; /* HDROP���擾 */
				vector<WCHAR> dropFile(255);
				DragQueryFile(hDrop, 0, &dropFile[0], 256); /* �ŏ��̃t�@�C�������擾 */
				DragFinish(hDrop); /* �h���b�v�̏I������ */
				Poco::UnicodeConverter::toUTF8(&dropFile[0], _interruptFile);
			}
			break;

		case WM_DEVICECHANGE:
			{
				bool addData = false;
				wstring ws;
				switch (wParam) {
				case DBT_CONFIGCHANGECANCELED:
					ws = L"DBT_CONFIGCHANGECANCELED�F�ݒ�ύX�v�����L�����Z������܂���";
					break;

				case DBT_CONFIGCHANGED:
					ws = L"DBT_CONFIGCHANGED�F�ݒ肪�ύX����܂���";
					break;

				case DBT_CUSTOMEVENT:
					ws = L"DBT_CUSTOMEVENT�F�h���C�o�[��`�̃J�X�^���C�x���g�����s����܂���";
					addData = true;
					break;

				case DBT_DEVICEARRIVAL:
					ws = L"DBT_DEVICEARRIVAL�F�f�o�C�X���g�p�\�ɂȂ�܂���";
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
					ws = L"DBT_DEVICEQUERYREMOVE�F�f�o�C�X��~�v�������s����܂���";
					addData = true;
					break;

				case DBT_DEVICEQUERYREMOVEFAILED:
					ws = L"DBT_DEVICEQUERYREMOVEFAILED�F�f�o�C�X��~�v�������s����܂���";
					addData = true;
					break;

				case DBT_DEVICEREMOVECOMPLETE:
					ws = L"DBT_DEVICEREMOVECOMPLETE�F�f�o�C�X����~����܂���";
					addData = true;
					break;

				case DBT_DEVICEREMOVEPENDING:
					ws = L"DBT_DEVICEREMOVEPENDING�F�f�o�C�X���~���ł�";
					addData = true;
					break;

				case DBT_DEVICETYPESPECIFIC:
					ws = L"DBT_DEVICETYPESPECIFIC�F�f�o�C�X�̓Ǝ��C�x���g�����s����܂���";
					addData = true;
					break;

				case DBT_DEVNODES_CHANGED:
					ws = L"DBT_DEVNODES_CHANGED�F�V�X�e���̃f�o�C�X��Ԃ��ω����܂���";
					break;

				case DBT_QUERYCHANGECONFIG:
					ws = L"DBT_QUERYCHANGECONFIG�F�ݒ�ύX�v�������s����܂���";
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


// GUI�̐ݒ�
bool guiConfiguration()
{
	// ffmpeg�̏�����
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
		// ���[�J�������w��
		fc->setProperty(Poco::FileChannel::PROP_TIMES, "local");
		// �A�[�J�C�u�t�@�C�����ւ̕t��������[number/timestamp] (���t)
		fc->setProperty(Poco::FileChannel::PROP_ARCHIVE, xml->getString("log[@archive]", "timestamp"));
		// ���k[true/false] (����)
		fc->setProperty(Poco::FileChannel::PROP_COMPRESS, xml->getString("log[@compress]", "true"));
		// ���[�e�[�V�����P��[never/[day,][hh]:mm/daily/weekly/monthly/<n>minutes/hours/days/weeks/months/<n>/<n>K/<n>M] (��)
		fc->setProperty(Poco::FileChannel::PROP_ROTATION, xml->getString("log[@rotation]", "daily"));
		// �ێ�����[<n>seconds/<n>minutes/<n>hours/<n>days/<n>weeks/<n>months] (5����)
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
			Poco::UnicodeConverter::toUTF8(L"�l�r �S�V�b�N", s);
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
			Poco::UnicodeConverter::toUTF8(L"�l�r �S�V�b�N", s);
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
		Poco::UnicodeConverter::toUTF8(L"�ݒ�t�@�C��(switch-config.xml)���m�F���Ă�������\n�u%s�v", s);
		wstring utf16;
		Poco::UnicodeConverter::toUTF16(Poco::format(s, ex.displayText()), utf16);
		::MessageBox(HWND_DESKTOP, utf16.c_str(), L"�G���[", MB_OK);
		_log.warning(ex.displayText());
	}
	return false;
}

Configuration& config() {
	return _conf;
}

// �X���b�v�A�E�g
void swapout() {
	_log.information("*** exec memory swapout");
	EmptyWorkingSet(GetCurrentProcess());
	return;

	DWORD idProcess[1024];
	DWORD bsize = 0;
	/* �v���Z�X���ʎq���擾���� */
	if (EnumProcesses(idProcess, sizeof(idProcess), &bsize) == FALSE) {
		_log.warning("failed EnumProcesses()");
		return;
	}
	_log.information("*** exec memory swapout");
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
					_log.information(Poco::format("process: %20s %05dkB%s", nameUTF8, mem, swapout));
				}
			}
			CloseHandle(handle);
		}
	}
}
