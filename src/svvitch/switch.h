#pragma once

#include "resource.h"

#include "Configuration.h"


/**
 * ���C�����[�v.
 */
void mainloop(HWND hWnd);

//LONG WINAPI ExceptionHandler(struct _EXCEPTION_POINTERS* pExceptionInfo);

/**
 * Window���b�Z�[�W�̏����֐�
 */
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/** �ݒ�I�u�W�F�N�g���擾���܂� */
Configuration& config();

/** �X���b�v�A�E�g���s���܂� */
void swapout();
