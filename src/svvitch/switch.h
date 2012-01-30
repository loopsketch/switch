#pragma once

#include "resource.h"

#include "Configuration.h"

/** @mainpage 
 * @section �݌v�ɂ���
 * �ȒP�Ȑ݌v�͎��̂Ƃ���B
 * - ::WinMain() ���S�̂̃G���g���|�C���g�ŁA�������Awindow�̐����A���C�����[�v�A�I���������s���܂��B
 * - {@link Renderer}�N���X���`��֌W�̋@�\���܂Ƃ߂��N���X�B<br>Direct3D�֘A�ADirectSound�֘A�̃��[�e�B���e�B�����̃N���X�ɂ܂Ƃ߂܂����B
 * - {@link Scene}�N���X��window��ɕ\�������\�����s���N���X�ł��B
 * - {@link Renderer#addScene()}�����{@link Renderer}�̊Ǘ����ƂȂ�A���t���[�� {@link Scene#process()}��{@link Scene#draw1()}��{@link Scene#draw2()} �ƌĂ΂�܂��B
 * - �T�C�l�[�W�̊�{���������Ă���̂�{@link MainScene}�ł��B
 * - {@link Content}�N���X�̓R���e���c�̒P�ʂƂ��A�摜�⓮��̕\���Ȃ�{@link MainScene}�̒��ŉ^�p�����\���I�u�W�F�N�g�̒P�ʂƂ��Ă��܂��B
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
