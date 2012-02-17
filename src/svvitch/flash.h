#pragma once

/**
 * Flashのコンポーネントを利用するためのヘッダ集
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
#import <C:\\Windows\\SysWOW64\\Macromed\\Flash\\Flash11e.ocx> named_guids
//#import <C:\\WINDOWS\\system32\\Macromed\\Flash\\Flash64_11_1_102.ocx> named_guids
#pragma warning(default: 4192)

using namespace ShockwaveFlashObjects;


typedef HRESULT (__stdcall *DllGetClassObjectFunc)(REFCLSID rclsid, REFIID riid, LPVOID * ppv);
