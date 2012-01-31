#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <Poco/Path.h>
#include <Poco/Channel.h>
#include <Poco/Logger.h>

using std::string;
using std::wstring;
using std::vector;
using Poco::Path;


/**
 * �ݒ�Ǘ��N���X.
 * XML����Ǎ��񂾒l��ێ����܂�
 */
class Configuration
{
private:
	Poco::Logger& _log;

public:
	/** ���O�o�͗p��Channel */
	Poco::Channel* logFile;

	/** window�^�C�g�� */
	string windowTitle;

	/** �f�B�X�v���C�� */
	string name;

	/** ���� */
	string description;

	/** ���C����ʗ̈� */
	RECT mainRect;

	/** ���C����ʍX�V���� */
	int mainRate;

	/** �T�u��ʗ̈� */
	RECT subRect;

	/** �T�u��ʍX�V���� */
	int subRate;

	/** ��ʍX�V�P�ʂ̃E�F�C�g */
	DWORD frameIntervals;

	/** window�t���[���g�p */
	bool frame;

	/** �t���X�N���[�����[�h�� */
	bool fullsceen;

	/** �N���b�v�̈���g�p���邩 */
	bool useClip;

	/** �N���b�v�̈� */
	RECT clipRect;

	/** �X�e�[�W�̈� */
	RECT stageRect;

	/** �X�i�b�v�V���b�g�p�̕i��(�k����)�ݒ� */
	float captureQuality;

	/** �X�i�b�v�V���b�g�p�̃t�B���^�[�ݒ� */
	string captureFilter;

	/** �����^�C�v */
	int splitType;

	/** �����T�C�Y */
	SIZE splitSize;

	/** �����J�Ԃ��� */
	int splitCycles;

	/** ���p����Đ��G���W�� */
	vector<string> movieEngines;

	/** ���p�V�[���� */
	vector<string> scenes;

	/** �X�e�[�W�̋P�x(0-100) */
	int brightness;

	/** �f�B�}�[(0-1.0) */
	float dimmer;

	/** �摜�̕�����px */
	bool viewStatus;

	/** �摜�̕�����px */
	int imageSplitWidth;

	/** �e�L�X�g�̃t�H���g */
	string textFont;

	/** �e�L�X�g�̃X�^�C�� */
	string textStyle;

	/** �e�L�X�g�̍��� */
	int textHeight;

	/** �}�E�X�̃J�[�\���\���� */
	bool mouse;

	/** window�̃h���b�N�� */
	bool draggable;

	/** �t�H���g�t�@�C�� */
	wstring defaultFont;

	/** ASCII�t�H���g�t�@�C�� */
	string asciiFont;

	/** �t�H���g�t�@�C�� */
	string multiByteFont;
	// string vpCommandFile;
	// string monitorFile;

	/** �f�[�^�f�B���N�g�����[�g */
	Path dataRoot;

	/** ���M�t�@�C���̈ꎞ�X�g�b�N�ꏊ */
	Path stockRoot;

	/** ���[�N�X�y�[�X�ݒu�p�X */
	Path workspaceFile;

	/** �j���[�XURL */
	string newsURL;

	/** �T�[�o�|�[�g */
	int serverPort;

	/** �T�[�o�L���[�� */
	int maxQueued;

	/** �T�[�o�X���b�h�� */
	int maxThreads;

	/** ���o���O���o�͂��邩 */
	bool outCastLog;



	/** �R���X�g���N�^ */
	Configuration();

	/** �f�X�g���N�^ */
	virtual ~Configuration();

	/** ������ */
	bool initialize();

	/** �ۑ� */
	void save();

	/** �w�肵���V�[�������ݒ�ɏ����Ă��邩�ǂ��� */
	bool hasScene(string s);

	/** �J�� */
	void release();
};

typedef Configuration* ConfigurationPtr;