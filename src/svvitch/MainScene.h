#pragma once

#include <Poco/StreamCopier.h>
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


struct PlayParameters
{
	string playlistID;
	int i;
	string action;
	string transition;
};

struct RemovableMediaArgs {
	const string& driveLetter;
};

class MainScene: public Scene
{
private:
	Poco::FastMutex _lock;
	Poco::FastMutex _workspaceLock;
	Poco::FastMutex _delayedUpdateLock;

	ui::UserInterfaceManager& _uim;
	Path& _workspaceFile;
	WorkspacePtr _workspace;
	WorkspacePtr _updatedWorkspace;

	DWORD _frame;
	bool _startup;
	bool _autoStart;
	int _brightness;

	vector<ContainerPtr> _contents;
	int _currentContent;
	bool _preparing;

	/** �Đ��R���e���c��� */
	PlayParameters _playCurrent;

	/** ���� */
	LPDIRECT3DTEXTURE9 _description;
	/** �v���C���X�g�� */
	LPDIRECT3DTEXTURE9 _playlistName;
	/** �Đ����̃R���e���c�� */
	LPDIRECT3DTEXTURE9 _currentName;

	vector<PlayParameters> _nextStack;
	int _nextStackTime;

	/** ���Đ��R���e���c��� */
	PlayParameters _playNext;

	/** ���̃v���C���X�g�� */
	LPDIRECT3DTEXTURE9 _nextPlaylistName;
	/** ���̃R���e���c�� */
	LPDIRECT3DTEXTURE9 _nextName;

	vector<PlayParameters> _prepareStack;
	int _prepareStackTime;

	/** �ؑ֏����R���e���c */
	PlayParameters _playPrepared;

	/** �ؑ֏����R���e���c��� */
	ContainerPtr _prepared;

	/** �ؑ֏����̃v���C���X�g�� */
	LPDIRECT3DTEXTURE9 _preparedPlaylistName;
	/** �ؑ֏����̃R���e���c�� */
	LPDIRECT3DTEXTURE9 _preparedName;

	/** �Đ��� */
	int _playCount;
	bool _doPrepareNext;
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
	float _removableIconAlpha;
	string _addRemovable;
	float _removableAlpha;
	float _removableCover;
	unsigned long _copySize;
	unsigned long _currentCopySize;
	int _copyProgress;
	int _currentCopyProgress;
	int _copyRemoteFiles;
	vector<File> _delayUpdateFiles;

	vector<ContainerPtr> _delayReleases;

	/** �X�^���o�C���f�B�A */
	map<string, ContainerPtr> _stanbyMedias;
	ContainerPtr _interrupttedContent;

	/** �X�^���o�C���f�B�A�̏��� */
	void preparedStanbyMedia();

	void run();

	/** �ؑ֗p�R���e���c�̏��� */
	bool prepareContent(const PlayParameters& args);

	/** ���Đ��R���e���c���������܂� */
	bool prepareNextContent(const PlayParameters& args);

	/** Container�Ɏw�肳�ꂽ�v���C���X�g�̃R���e���c���������܂� */
	bool preparePlaylist(ContainerPtr container, const string& playlistID, const int i = 0);

	bool prepareMedia(ContainerPtr container, MediaItemPtr media, const string& templatedText);

	void addRemovableMedia(const string& driveLetter);

	int copyFiles(const string& src, const string& dst);

	/** �����[�g�R�s�[ */
	void copyRemote(const string& remote);

	bool copyRemoteFile(const string& remote, const string& path, Path& out, bool equalityCheck = false);

	void setRemoteStatus(const string& remote, const string& name, const string& value);

	/** �����[�g�f�B���N�g���̃R�s�[ */
/*	bool copyRemoteDir(const string& remote, const string& root);
*/
	/** ���Đ��R���e���c������(�A�N�e�B�u��) */
	ActiveMethod<bool, PlayParameters, MainScene> activePrepareNextContent;


public:
	MainScene(Renderer& renderer, ui::UserInterfaceManager& uim, Path& workspaceFile);

	virtual ~MainScene();

	/** �R���e���c�̒x��������s���܂� */
	void delayedReleaseContainer();

	/** ������ */
	bool initialize();

	/** �ݒ肳��Ă���Workspace���擾���܂� */
	Workspace& getWorkspace();

	void notifyKey(const int keycode, const bool shift, const bool ctrl);

	/** �ؑ֗p�R���e���c���X�^�b�N���܂� */
	bool stackPrepareContent(string& playlistID, int i = 0);

	/** �����ݒ� */
	const void setDescription(const string& description);

	/** �v���C���X�g�e�L�X�g�擾 */
	const string getPlaylistText(const string& playlistID);

	/** �v���C���X�g�e�L�X�g�ݒ� */
	bool setPlaylistText(const string& playlistID, const string& text);

	/** �P�x�ݒ� */
	void setBrightness(int i);

	/** �J�ڃA�N�V�����ݒ� */
	void setAction(string& action);

	/** �g�����W�V�����ݒ� */
	void setTransition(string& transition);

	/** �ؑ֗p�R���e���c�̏���(�A�N�e�B�u��) */
	ActiveMethod<bool, PlayParameters, MainScene> activePrepareContent;

	/**
	 * �蓮�Őؑւ��s���܂�
	 * ���̃��\�b�h�̓��C���X���b�h���u���b�N����̂Ń��C���X���b�h�����activeSwitchContent()�Ōďo������
	 */
	bool switchContent();

	/** �ؑ֗p�R���e���c�ɐؑ�(�A�N�e�B�u��) */
	ActiveMethod<bool, void, MainScene> activeSwitchContent;

	/** �x���X�V�t�@�C����ǉ����܂� */
	void addDelayedUpdateFile(File& file);

	/** �x���X�V�t�@�C����ǉ����܂� */
	void removeDelayedUpdateFile(File& file);

	/** �x���X�V�t�@�C�����X�V���܂� */
	void updateDelayedFiles();

	/** workspace�X�V */
	bool updateWorkspace();

	/** �����[�g�R�s�[ (�A�N�e�B�u��) */
	ActiveMethod<void, string, MainScene> activeCopyRemote;

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
