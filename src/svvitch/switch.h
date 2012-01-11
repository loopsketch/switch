#pragma once

#include "resource.h"

#include "Configuration.h"


/**
 * メインループ.
 */
void mainloop(HWND hWnd);

//LONG WINAPI ExceptionHandler(struct _EXCEPTION_POINTERS* pExceptionInfo);

/**
 * Windowメッセージの処理関数
 */
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/** 設定オブジェクトを取得します */
Configuration& config();

/** スワップアウトを行います */
void swapout();
