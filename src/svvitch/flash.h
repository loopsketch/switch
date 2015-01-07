#pragma once

/**
 * Flash�̃R���|�[�l���g�𗘗p���邽�߂̃w�b�_�W
 */
#ifdef UNICODE
#define FormatMessage FormatMessageW
#define FindResource FindResourceW
#define GetModuleFileName GetModuleFileNameW
#define CreateFile CreateFileW
#define LoadLibrary LoadLibraryW
#define CreateEvent CreateEventW
#else
#define FormatMessage FormatMessageA
#define FindResource FindResourceA
#define GetModuleFileName GetModuleFileNameA
#define CreateFile CreateFileA
#define LoadLibrary LoadLibraryA
#define CreateEvent CreateEventA
#endif // !UNICODE

#include <windows.h>
#include <atlbase.h>

#pragma warning(disable: 4192)
#import <C:\\Windows\\SysWOW64\\Macromed\\Flash\\Flash32_15_0_0_246.ocx> named_guids
//#import <C:\\Windows\\SysWOW64\\Macromed\\Flash\\Flash32_11_7_700_224.ocx> named_guids
//#import <C:\\WINDOWS\\system32\\Macromed\\Flash\\Flash64_11_1_102.ocx> named_guids
#pragma warning(default: 4192)

using namespace ShockwaveFlashObjects;


typedef HRESULT (__stdcall *DllGetClassObjectFunc)(REFCLSID rclsid, REFIID riid, LPVOID * ppv);
