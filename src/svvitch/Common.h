#pragma once
//
// Common.h
// ���ʂŎg�p����}�N����萔�̒�`
//

#include <windows.h>
#include <stdio.h> //for sprintf

typedef char					Sint8;								///< signed char �^�̕ʒ�`
typedef short					Sint16;								///< signed short �^�̕ʒ�`
typedef long					Sint32;								///< signed long �^�̕ʒ�`
typedef __int64					Sint64;								///< signed __int64 �^�̕ʒ�`
typedef unsigned char			Uint8;								///< unsigned char �^�̕ʒ�`
typedef unsigned short			Uint16;								///< unsigned short �^�̕ʒ�`
typedef unsigned long			Uint32;								///< unsigned long �^�̕ʒ�`
typedef unsigned __int64		Uint64;								///< unsigned __int64 �^�̕ʒ�`
typedef float					Float;								///< Float �^�̕ʒ�`
typedef float					Float32;							///< Float �^�̕ʒ�`
typedef double					Float64;							///< double �^�̕ʒ�`
typedef bool					Bool;								///< Bool �^�̕ʒ�`

#define toF(V)					((Float)(V))																///< Float�^�ւ̃L���X�g�}�N��
#define toI(V)					((Sint32)(V))																///< Sint32�^�ւ̃L���X�g�}�N��
#define F(V)					toF(V)
#define L(V)					toI(V)

#define PI						(3.141592653589793238462643383279f)											///< ��
#define PI2						(6.283185307179586476925286766559f)											///< 2��
#define REV(V)					toF(1.0f/toF(V))															///< �t���Z�o�}�N��

// �������̉��
#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }

// �Q�ƃJ�E���^�̃f�N�������g
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

// �G���[�̕񍐂ƃA�v���P�[�V�����̏I��
#define ERROR_EXIT() { int line = __LINE__; const char *file = __FILE__;\
	char msg[_MAX_FNAME + _MAX_EXT + 256];\
	char drive[_MAX_DRIVE];\
	char dir[_MAX_DIR];\
	char fname[_MAX_FNAME];\
	char ext[_MAX_EXT];\
	_splitpath(file, drive, dir, fname, ext);\
	sprintf(msg, "���炩�̃G���[�������������߃A�v���P�[�V�������I�����܂�\r\n"\
		"�t�@�C�� : %s%s\r\n"\
		"�s�ԍ� : %d", fname, ext, line);\
	MessageBox(NULL, msg, "Error", MB_OK | MB_ICONEXCLAMATION);\
	PostQuitMessage(1);\
}
