#pragma once

#include <queue>
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>

#include "Content.h"
#include "CaptureScene.h"
#include "MediaItem.h"
#include "PerformanceTimer.h"
#include "Renderer.h"
#include "Workspace.h"

using std::queue;
using std::string;
using std::wstring;


/**
 * �L���v�`���[�R���e���g�N���X.
 * CaptureScene ����L���v�`���[�f�����擾���`�悷�� Content �N���X�ł��B
 */
class CaptureContent: public Content
{
private:
	Poco::FastMutex _lock;

	CaptureScenePtr _scene;

	float _subtract;
	int _intervalDiff;
	int _intervalSmall;

	LPD3DXEFFECT _fx;
	LPDIRECT3DTEXTURE9 _small1;
	LPDIRECT3DTEXTURE9 _small2;
	LPDIRECT3DTEXTURE9 _diff;
	LPDIRECT3DSURFACE9 _diff2;
	LPDIRECT3DTEXTURE9 _image;
	int _detectThreshold;
	int _diffCount;

	bool _detected;
	int _doShutter;
	int _viewPhoto;

	bool _finished;
	bool _playing;
	PerformanceTimer _playTimer;

	DWORD _statusFrame;
	string _status;

public:
	/** �R���X�g���N�^ */
	CaptureContent(Renderer& renderer, int splitType);

	/** �f�X�g���N�^ */
	~CaptureContent();


	/** �ݒ��ۑ����܂� */
	void saveConfiguration();

	/** ������ */
	void initialize();

	/** �t�@�C�����I�[�v�����܂� */
	bool open(const MediaItemPtr media, const int offset = 0);


	/** �Đ� */
	void play();

	/** ��~ */
	void stop();

	/** �Đ��I�����ɂ������ܒ�~���邩�ǂ��� */
	bool useFastStop();

	/** �Đ������ǂ��� */
	const bool playing() const;

	/** �I���������ǂ��� */
	const bool finished();

	/** �N���[�Y���܂� */
	void close();

	/** �`��ȊO�̏��� */
	void process(const DWORD& frame);

	/** �`�揈�� */
	void draw(const DWORD& frame);
};

typedef CaptureContent* CaptureContentPtr;