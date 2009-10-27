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

	/** プレイリスト名 */
	LPDIRECT3DTEXTURE9 _playlistName;
	/** 再生中のコンテンツ名 */
	LPDIRECT3DTEXTURE9 _currentName;
	/** 準備済のコンテンツ名 */
	LPDIRECT3DTEXTURE9 _preparedName;
	/** 準備済のコマンド */
	string _preparedCommand;
	/** 準備済のトランジション */
	string _preparedTransition;

	/** 再生回数 */
	int _playCount;
	/** 切替フラグ */
	bool _doSwitch;
	/** 切替を抑制するフラグ.true時には切替を受付けない.例>次コンテンツ準備中など */
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

	/** containerの準備を行います */
	bool prepareMedia(ContainerPtr container, const string& playlistID, const int i = 0);

	/** 手動で切替を行います */
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