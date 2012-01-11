#pragma once

#include "resource.h"

#include "Configuration.h"

/** @mainpage 
 * @section �\�[�X�R�[�h�ɂ���
 * - �G���g���[�|�C���g > ::WinMain()
 * .
 * @section �֘A�T�C�g
 * - @b ProjectHome http://sourceforge.jp/projects/switch/
 **/

/**
 * �A�v���P�[�V�����̃G���g���|�C���g�ł�
 * @param	hInstance		���݂̃C���X�^���X�̃n���h��
 * @param	hPrevInstance	�ȑO�̃C���X�^���X�̃n���h��
 * @param	lpCmdLine		�R�}���h���C���p�����[�^
 * @param	nCmdShow		�E�B���h�E�̕\�����
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

/** ���C�����[�v */
void mainloop(HWND hWnd);

//LONG WINAPI ExceptionHandler(struct _EXCEPTION_POINTERS* pExceptionInfo);

/**  Window���b�Z�[�W�̏��� */
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/** �ݒ�I�u�W�F�N�g���擾���܂� */
Configuration& config();

/** �X���b�v�A�E�g���s���܂� */
void swapout();
