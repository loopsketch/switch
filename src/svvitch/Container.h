#pragma once

#include "Content.h"
#include <Poco/Logger.h>
#include <Poco/Mutex.h>
#include <vector>

using std::vector;


/**
 * ������Content���܂�Content�̃R���e�i
 */
class Container: public Content
{
private:
	Poco::FastMutex _lock;
	vector<ContentPtr> _list;

	bool _initialized;

public:
	/** �R���X�g���N�^ */
	Container(Renderer& renderer);

	/** �f�X�g���N�^ */
	virtual ~Container();

	/** ������ */
	void initialize();

	/**
	 * �R���e�i��Content��ǉ����܂�
	 * @param c	�ǉ�����R���e���c
	 */
	void add(ContentPtr c);

	/** �z��ŃA�N�Z�X */
	ContentPtr operator[](int i);

	/**
	 * �w�肵���C���f�b�N�X�̃R���e���c��Ԃ��܂�
	 * @param i	�C���f�b�N�X�ԍ�
	 */
	ContentPtr get(int i);

	/** �R���e���c�� */
	int size();

	/**
	 * open�ς��ǂ���
	 * @return	�I�[�v�����Ă���R���e���cID
	 */
	const string opened();

	/** �Đ� */
	void play();

	/** �|�[�Y */
	void pause();

	/** ��~ */
	void stop();

	/** ��~���ɂ������ܒ�~���邩�ǂ��� */
	bool useFastStop();

	/** ���o�� */
	void rewind();

	/**
	 * �I���������ǂ���
	 * @return	�I�����Ă����true�A�܂��Ȃ�false
	 */
	const bool finished();

	/**
	 * �L�[�ʒm
	 * @param	keycide	�����L�[�R�[�h
	 * @param	shift	SHIFT�L�[�̉����t���O
	 * @param	ctrl	CTRL�L�[�̉����t���O
	 */
	void notifyKey(const int keycode, const bool shift, const bool ctrl);

	/** �`��ȊO�̏��� */
	void process(const DWORD& frame);

	/** �`�揈�� */
	void draw(const DWORD& frame);

	/** �v���r���[���� */
	void preview(const DWORD& frame);

	/** ���݂̍Đ��t���[�� */
	const int current();

	/** �t���[���� */
	const int duration();

	/** �v���p�e�B�̐ݒ� */
	void setProperty(const string& key, const string& value);
};

typedef Container* ContainerPtr;