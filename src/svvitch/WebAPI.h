#pragma once

#include "HTTPServerBase.h"
#include "Renderer.h"


/**
 * HTTP���N�G�X�g�n���h��
 */
class SwitchRequestHandler: public BaseRequestHandler {
private:
	Renderer& _renderer;

	/** �ؑ� */
	void switchContent();

	/** �X�V */
	void updateWorkspace();

	/** �ݒ�n */
	void set(const string& name);

	/** �擾�n */
	void get(const string& name);

	/**
	 * �t�@�C������Ԃ��܂�.
	 * �t�@�C���̃p�X���w�肵���ꍇ�̓t�@�C������map��.
	 * �f�B���N�g�����w�肵���ꍇ�͎q�̃t�@�C����&�f�B���N�g�����̔z���.
	 */
	void files();

	/** �t�@�C����JSON������ */
	string fileToJSON(const Path path);

	/** �_�E�����[�h */
	void download();

	/** �A�b�v���[�h */
	void upload();

	/** �X�g�b�N�t�@�C�����N���A���܂� */
	void clearStock();

	/** �R�s�[ */
	void copy();

	/** �o�[�W���� */
	void version();


protected:
	void doRequest();


public:
	SwitchRequestHandler(Renderer& renderer);

	virtual ~SwitchRequestHandler();
};


/**
 * HTTP���N�G�X�g�n���h���p�t�@�N�g��
 */
class SwitchRequestHandlerFactory: public HTTPRequestHandlerFactory {	
private:
	Poco::Logger& _log;
	Renderer& _renderer;

public:
	SwitchRequestHandlerFactory(Renderer& renderer);

	virtual ~SwitchRequestHandlerFactory();

	HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request);
};
