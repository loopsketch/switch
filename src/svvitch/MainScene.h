#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <Poco/ActiveMethod.h>
#include <Poco/ActiveResult.h>
#include <Poco/Logger.h>
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>
#include <map>
#include <string>
#include <vector>
#include <queue>

#include "Common.h"

#include "Container.h"
#include "Renderer.h"
#include "Scene.h"
#include "Transition.h"
#include "ui/UserInterfaceManager.h"
#include "Workspace.h"

using std::string;
using std::vector;
using std::queue;
using std::map;

using Poco::ActiveMethod;
using Poco::ActiveResult;


struct PrepareArgs
{
	string playlistID;
	int i;
};


class MainScene: public Scene
{
private:
	Poco::FastMutex _lock;
	Poco::FastMutex _workspaceLock;

	ui::UserInterfaceManager& _uim;
	Path& _workspaceFile;
	WorkspacePtr _workspace;
	WorkspacePtr _updatedWorkspace;

	DWORD _frame;
	bool _startup;
	bool _autoStart;
	string _action;
	int _luminance;

	string _playlistID;
	int _playlistItem;

	MediaItemPtr _interruptMedia;

	vector<ContainerPtr> _contents;
	int _currentContent;
	bool _preparing;

	vector<PrepareArgs> _prepareStack;
	int _prepareStackTime;

	/** �v���C���X�g�� */
	LPDIRECT3DTEXTURE9 _playlistName;
	/** �Đ����̃R���e���c�� */
	LPDIRECT3DTEXTURE9 _currentName;
	/** ���̃v���C���X�g�� */
	LPDIRECT3DTEXTURE9 _nextPlaylistName;
	/** ���̃R���e���c�� */
	LPDIRECT3DTEXTURE9 _nextName;
	/** ���̃A�N�V���� */
	string _nextAction;
	/** ���̃g�����W�V���� */
	string _nextTransition;

	/** �ؑ֏����R���e���c */
	ContainerPtr _prepared;
	/** �ؑ֏����v���C���X�g */
	string _preparedPlaylistID;
	/** �ؑ֏����v���C���X�g�A�C�e�� */
	int _preparedItem;
	/** �ؑ֏����̃v���C���X�g�� */
	LPDIRECT3DTEXTURE9 _preparedPlaylistName;
	/** �ؑ֏����̃R���e���c�� */
	LPDIRECT3DTEXTURE9 _preparedName;

	/** �Đ��� */
	int _playCount;
	bool _preparingNext;
	/** �ؑփt���O */
	bool _doSwitchNext;
	/** �ؑ֏����R���e���c�̐ؑփt���O */
	bool _doSwitchPrepared;
	/** �g�����W�V���� */
	TransitionPtr _transition;

	/** ���ݎ��� */
	string _nowTime;
	/** ���ݎ����̕b */
	int _timeSecond;

	bool _initializing;
	bool _running;

	/** USB�A�C�R�� */
	LPDIRECT3DTEXTURE9 _removableIcon;
	string _addRemovable;
	float _removableAlpha;
	float _removableCover;
	unsigned long _copySize;
	unsigned long _currentCopySize;
	int _copyProgress;
	int _currentCopyProgress;

	vector<LPVOID> _lateDeletes;
	vector<LPDIRECT3DTEXTURE9> _lateRleaseTextures;

	void run();

	/** �ؑ֗p�R���e���c�̏��� */
	bool prepare(const PrepareArgs& args);

	/** ���Đ��R���e���c���������܂� */
	bool prepareNextMedia();

	/** Container�Ɏw�肳�ꂽ�v���C���X�g�̃R���e���c���������܂� */
	bool prepareMedia(ContainerPtr container, const string& playlistID, const int i = 0);

	void addRemovableMedia(const string& driveLetter);

	int copyFiles(const string& src, const string& dst);

	/** �I�u�W�F�N�g�̉�� */
	void deleteObjects(vector<LPVOID>& objects);

	/** �e�N�X�`���̉�� */
	void releaseTextures(vector<LPDIRECT3DTEXTURE9>& textures);

public:
	MainScene(Renderer& renderer, ui::UserInterfaceManager& uim, Path& workspaceFile);

	virtual ~MainScene();

	/** ������ */
	bool initialize();

	/** �ݒ肳��Ă���Workspace���擾���܂� */
	Workspace& getWorkspace();

	void notifyKey(const int keycode, const bool shift, const bool ctrl);

	/** �ؑ֗p�R���e���c���X�^�b�N���܂� */
	bool stackPrepare(string& playlistID, int i = 0);

	/** �v���C���X�g�e�L�X�g�ݒ� */
	bool setPlaylistText(string& playlistID, string& text);

	/** �P�x�ݒ� */
	void setLuminance(int i);

	/** �J�ڃA�N�V�����ݒ� */
	void setAction(string& action);

	/** �g�����W�V�����ݒ� */
	void setTransition(string& transition);

	/** �ؑ֗p�R���e���c�̏���(�A�N�e�B�u��) */
	ActiveMethod<bool, PrepareArgs, MainScene> activePrepare;

	/** �蓮�Őؑւ��s���܂� */
	bool switchContent();

	/** workspace�X�V */
	bool updateWorkspace();

	/** ���Đ��R���e���c������(�A�N�e�B�u��) */
	ActiveMethod<bool, void, MainScene> activePrepareNextMedia;

	/** �����[�o�u�����f�B�A�̒ǉ�(�A�N�e�B�u��) */
	ActiveMethod<void, string, MainScene> activeAddRemovableMedia;

	/** ���t���[���ōs������ */
	virtual void process();

	/**
	 * �t���[���`��
	 * �R���e���c�Ȃǂ̃��C���`��n
	 */
	virtual void draw1();

	/**
	 * �t���[���`��
	 * ��ɃX�e�[�^�X�n
	 */
	virtual void draw2();
};

typedef MainScene* MainScenePtr;
