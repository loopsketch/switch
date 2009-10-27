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

	ui::UserInterfaceManager* _uim;
	WorkspacePtr _workspace;

	DWORD _frame;
	int _luminance;
	bool _startup;
	string _currentCommand;

	string _playlistID;
	int _playlistItem;

	MediaItemPtr _interruptMedia;

	vector<ContainerPtr> _contents;
	int _currentContent;

	/** �v���C���X�g�� */
	LPDIRECT3DTEXTURE9 _playlistName;
	/** �Đ����̃R���e���c�� */
	LPDIRECT3DTEXTURE9 _currentName;
	/** �����ς̃R���e���c�� */
	LPDIRECT3DTEXTURE9 _preparedName;
	/** �����ς̃R�}���h */
	string _preparedCommand;
	/** �����ς̃g�����W�V���� */
	string _preparedTransition;

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

	void goMenuScene();

	void goOperationScene();

	void goEditScene();

	void goSetupScene();


public:
	MainScene(Renderer& renderer, ui::UserInterfaceManagerPtr uim);

	~MainScene();

	virtual bool initialize();

	virtual bool setWorkspace(WorkspacePtr workspace);

	virtual void notifyKey(const int keycode, const bool shift, const bool ctrl);

	/** container�̏������s���܂� */
	bool prepareMedia(ContainerPtr container, const string& playlistID, const int i = 0);

	/** �蓮�Őؑւ��s���܂� */
	void switchContent(ContainerPtr* container, const string& playlistID, const int i = 0);

	Poco::ActiveMethod<bool, void, MainScene> activePrepareNextMedia;

	Poco::ActiveMethod<void, void, MainScene> activeGoMenuScene;

	Poco::ActiveMethod<void, void, MainScene> activeGoOperation;

	Poco::ActiveMethod<void, void, MainScene> activeGoEdit;

	Poco::ActiveMethod<void, void, MainScene> activeGoSetup;


	virtual void process();

	virtual void draw1();

	virtual void draw2();
};

typedef MainScene* MainScenePtr;