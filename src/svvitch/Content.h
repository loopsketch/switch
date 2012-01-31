#pragma once

#include <string>
#include <vector>
#include <Poco/ActiveMethod.h>
#include <Poco/format.h>
#include <Poco/Logger.h>
#include <Poco/HashMap.h>

#include "Renderer.h"
#include "MediaItem.h"

using std::string;
using std::vector;
using std::wstring;
using Poco::HashMap;
using Poco::ActiveMethod;


/**
 * �R���e���g�N���X.
 * <Scene>�̒��ŕ`�悳���ۂ̍ŏ��P�ʂł��B
 * �ėp�I�ȃR���|�[�l���g�Ƃ��ė��p���������̂�<Content>�ɂ��ׂ��ł�
 */
class Content
{
private:
	//static const string NULL_STRING;

protected:
	Poco::Logger& _log;
	Renderer& _renderer;

	int _splitType;
//	MediaItemPtr _media;
	string _mediaID;
	bool _playing;

	int _duration;
	int _current;

	int _keycode;
	bool _shift;
	bool _ctrl;

	HashMap<string, string> _properties;
	float _x, _y, _w, _h;

public:
	/** �R���X�g���N�^ */
	Content(Renderer& renderer, int splitType, float x = 0, float y = 0, float w = 0, float h = 0);

	/** �f�X�g���N�^ */
	virtual ~Content();

	/** ������ */
	virtual void initialize();

	/** �t�@�C�����I�[�v�����܂� */
	virtual bool open(const MediaItemPtr media, const int offset = 0);

	/** �t�@�C���̏������������Ă��邩�ǂ��� */
	virtual const string opened() const;

	/** �Đ� */
	virtual void play();

	/** �|�[�Y */
	virtual void pause();

	/** ��~ */
	virtual void stop();

	/** �Đ��I�����ɂ������ܒ�~���邩�ǂ��� */
	virtual bool useFastStop();

	/** ���������� */
	virtual void rewind();

	/** �Đ������ǂ��� */
	virtual const bool playing() const;

	/** �I���������ǂ��� */
	virtual const bool finished();

	/** �N���[�Y���܂� */
	virtual void close();

	ActiveMethod<void, void, Content> activeClose;

	/**
	 * �L�[�ʒm
	 * @param	keycide	�����L�[�R�[�h
	 * @param	shift	SHIFT�L�[�̉����t���O
	 * @param	ctrl	CTRL�L�[�̉����t���O
	 */
	void notifyKey(const int keycode, const bool shift, const bool ctrl);

	/** 1�t���[����1�x������������� */
	virtual void process(const DWORD& frame);

	/** �`�� */
	virtual void draw(const DWORD& frame);

	/** �v���r���[�`�� */
	virtual void preview(const DWORD& frame);

	/** ���݂̃t���[�� */
	virtual const int current() const;

	/** ����(�t���[����) */
	virtual const int duration() const;

	/** �ʒu�ݒ� */
	virtual void setPosition(float x, float y);

	/** �ʒu�擾 */
	virtual void getPosition(float& x, float& y);

	/** �̈�ݒ� */
	virtual void setBounds(float w, float h);

	/** �w��ʒu���̈�͈͓����ǂ��� */
	virtual const bool contains(float x, float y) const;

	/** �p�����[�^��ݒ肵�܂� */
	void set(const string& key, const string& value);

	/** �p�����[�^��ݒ肵�܂� */
	void set(const string& key, const float& value);

	/** �p�����[�^��ݒ肵�܂� */
	void set(const string& key, const unsigned int& value);

	/** �p�����[�^���擾���܂� */
	const string& get(const string& key, const string& defaultValue = "") const;

	/** �p�����[�^���擾���܂� */
	const DWORD getDW(const string& key, const DWORD& defaultValue = 0) const;

	/** �p�����[�^���擾���܂� */
	const int getI(const string& key, const int& defaultValue = 0) const;

	/** �p�����[�^���擾���܂� */
	const float getF(const string& key, const float& defaultValue = 0) const;
};

typedef Content* ContentPtr;
