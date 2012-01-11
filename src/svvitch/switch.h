#pragma once

#include "resource.h"

#include "Configuration.h"

/** @mainpage 
 * @section ソースコードについて
 * - エントリーポイント > ::WinMain()
 * .
 * @section 関連サイト
 * - @b ProjectHome http://sourceforge.jp/projects/switch/
 **/

/**
 * アプリケーションのエントリポイントです
 * @param	hInstance		現在のインスタンスのハンドル
 * @param	hPrevInstance	以前のインスタンスのハンドル
 * @param	lpCmdLine		コマンドラインパラメータ
 * @param	nCmdShow		ウィンドウの表示状態
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

/** メインループ */
void mainloop(HWND hWnd);

//LONG WINAPI ExceptionHandler(struct _EXCEPTION_POINTERS* pExceptionInfo);

/**  Windowメッセージの処理 */
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/** 設定オブジェクトを取得します */
Configuration& config();

/** スワップアウトを行います */
void swapout();
