#pragma once
//
// Common.h
// 共通で使用するマクロや定数の定義
//

#include <windows.h>
#include <stdio.h> //for sprintf

typedef char					Sint8;								///< signed char 型の別定義
typedef short					Sint16;								///< signed short 型の別定義
typedef long					Sint32;								///< signed long 型の別定義
typedef __int64					Sint64;								///< signed __int64 型の別定義
typedef unsigned char			Uint8;								///< unsigned char 型の別定義
typedef unsigned short			Uint16;								///< unsigned short 型の別定義
typedef unsigned long			Uint32;								///< unsigned long 型の別定義
typedef unsigned __int64		Uint64;								///< unsigned __int64 型の別定義
typedef float					Float;								///< Float 型の別定義
typedef float					Float32;							///< Float 型の別定義
typedef double					Float64;							///< double 型の別定義
typedef bool					Bool;								///< Bool 型の別定義

#define toF(V)					((Float)(V))																///< Float型へのキャストマクロ
#define toI(V)					((Sint32)(V))																///< Sint32型へのキャストマクロ
#define F(V)					toF(V)
#define L(V)					toI(V)

#define PI						(3.141592653589793238462643383279f)											///< π
#define PI2						(6.283185307179586476925286766559f)											///< 2π
#define REV(V)					toF(1.0f/toF(V))															///< 逆数算出マクロ

// メモリの解放
#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }

// 参照カウンタのデクリメント
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

// エラーの報告とアプリケーションの終了
#define ERROR_EXIT() { int line = __LINE__; const char *file = __FILE__;\
	char msg[_MAX_FNAME + _MAX_EXT + 256];\
	char drive[_MAX_DRIVE];\
	char dir[_MAX_DIR];\
	char fname[_MAX_FNAME];\
	char ext[_MAX_EXT];\
	_splitpath(file, drive, dir, fname, ext);\
	sprintf(msg, "何らかのエラーが発生したためアプリケーションを終了します\r\n"\
		"ファイル : %s%s\r\n"\
		"行番号 : %d", fname, ext, line);\
	MessageBox(NULL, msg, "Error", MB_OK | MB_ICONEXCLAMATION);\
	PostQuitMessage(1);\
}
