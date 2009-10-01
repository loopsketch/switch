#pragma once

#include "resource.h"


LONG WINAPI ExceptionHandler(struct _EXCEPTION_POINTERS* pExceptionInfo);

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool guiConfiguration(void);

void swapout(void);
