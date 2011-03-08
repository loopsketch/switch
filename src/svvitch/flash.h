#pragma once

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
#import <C:\\WINDOWS\\system32\\Macromed\\Flash\\Flash10n.ocx> named_guids
#pragma warning(default: 4192)

using namespace ShockwaveFlashObjects;


typedef HRESULT (__stdcall *DllGetClassObjectFunc)(REFCLSID rclsid, REFIID riid, LPVOID * ppv);
