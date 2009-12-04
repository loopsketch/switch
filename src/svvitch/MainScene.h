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



class MainScene: public Scene
{
private:
	Poco::FastMutex _lock;
	Poco::FastMutex _switchLock;

	ui::UserInterfaceManager& _uim;
	Workspace& _workspace;

	DWORD _frame;
	int _luminance;
	bool _startup;
	string _currentCommand;

	string _playlistID;
	int _playlistItem;

	MediaItemPtr _interruptMedia;

	vector<ContainerPtr> _contents;
	int _currentContent;
	bool _preparing;

	/** �v���C���X�g�� */
	LPDIRECT3DTEXTURE9 _playlistName;
	/** �Đ����̃R���e���c�� */
	LPDIRECT3DTEXTURE9 _currentName;
	/** �����ς̃v���C���X�g�� */
	LPDIRECT3DTEXTURE9 _nextPlaylistName;
	/** �����ς̃R���e���c�� */
	LPDIRECT3DTEXTURE9 _nextName;
	/** �����ς̃R�}���h */
	string _nextCommand;
	/** �����ς̃g�����W�V���� */
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
	/** �ؑփt���O */
	bool _doSwitch;
	/** �ؑւ�}������t���O.true���ɂ͐ؑւ���t���Ȃ�.��>���R���e���c�������Ȃ� */
	bool _suppressSwitch;
	TransitionPtr _transition;


	void run();

	void updateContentList();

	void prepareContent();

	bool prepareNextMedia();


public:
	MainScene(Renderer& renderer, ui::UserInterfaceManager& uim, Workspace& workspace);

	virtual ~MainScene();

	/** ������ */
	bool initialize();

	Workspace& getWorkspace();

	void notifyKey(const int keycode, const bool shift, const bool ctrl);

	/** container�̏������s���܂� */
	bool prepare(const string& playlistID, const int i = 0);

	/** container�̏������s���܂� */
	bool prepareMedia(ContainerPtr container, const string& playlistID, const int i = 0);

	/** �蓮�Őؑւ��s���܂� */
	void switchContent();

	Poco::ActiveMethod<bool, void, MainScene> activePrepareNextMedia;


	virtual void process();

	virtual void draw1();

	virtual void draw2();
};

typedef MainScene* MainScenePtr;
