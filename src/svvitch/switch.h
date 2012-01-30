#pragma once

#include "resource.h"

#include "Configuration.h"

/** @mainpage 
 * @section 設計について
 * 簡単な設計は次のとおり。
 * - ::WinMain() が全体のエントリポイントで、初期化、windowの生成、メインループ、終了処理が行われます。
 * - {@link Renderer}クラスが描画関係の機能をまとめたクラス。<br>Direct3D関連、DirectSound関連のユーティリティもこのクラスにまとめました。
 * - {@link Scene}クラスはwindow上に表示される表現を行うクラスです。
 * - {@link Renderer#addScene()}すると{@link Renderer}の管理下となり、毎フレーム {@link Scene#process()}⇒{@link Scene#draw1()}⇒{@link Scene#draw2()} と呼ばれます。
 * - サイネージの基本処理をしているのは{@link MainScene}です。
 * - {@link Content}クラスはコンテンツの単位とし、画像や動画の表示など{@link MainScene}の中で運用される表示オブジェクトの単位としています。
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
