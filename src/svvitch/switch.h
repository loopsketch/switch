#pragma once

#include "resource.h"

#include "Configuration.h"


LONG WINAPI ExceptionHandler(struct _EXCEPTION_POINTERS* pExceptionInfo);

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool guiConfiguration();

/**
 * �ݒ�I�u�W�F�N�g���擾���܂�
 */
Configuration& config();

void swapout();
