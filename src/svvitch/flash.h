#pragma once

//#include "rpc.h"
//#include "rpcndr.h"

#ifdef UNICODE
#define FormatMessage FormatMessageW
#else
#define FormatMessage FormatMessageA
#endif // !UNICODE

#pragma warning(disable: 4192)
#import "C:\\WINDOWS\\system32\\macromed\\Flash\\Flash10e.ocx" named_guids
#pragma warning(default: 4192)

using namespace ShockwaveFlashObjects;


typedef HRESULT (__stdcall *DllGetClassObjectFunc)(REFCLSID rclsid, REFIID riid, LPVOID * ppv);
