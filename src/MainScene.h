#pragma once

#include "Scene.h"
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
#include "Workspace.h"
#include "Transition.h"

using std::string;
using std::vector;
using std::queue;


class MainScene: public Scene
{
private:
	Poco::FastMutex _lock;
#if defined(DEBUG) | defined(_DEBUG)
	_CrtMemState s1;
#endif

	WorkspacePtr _workspace;

	DWORD _frame;
	int _luminance;
	bool _startup;
	int _currentItem;
	string _currentCommand;

	PlayListPtr _currentPlaylist;
	PlayListItemPtr _nextItem;
	MediaItemPtr _interruptMedia;

	vector<ContainerPtr> _contents;
	int _currentContent;

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

	Poco::ActiveMethod<void, void, MainScene> activeCloseNextMedia;
	void closeNextMedia();

	Poco::ActiveMethod<void, void, MainScene> activePrepareNextMedia;
	void prepareNextMedia();

public:

	MainScene(Renderer& renderer);

	~MainScene();

	virtual bool initialize();

	virtual bool setWorkspace(WorkspacePtr workspace);

	virtual void notifyKey(const int keycode, const bool shift, const bool ctrl);

	void switchContent(ContainerPtr* container, PlayListPtr playlist, const int content = 0);

	virtual void process();

	virtual void draw1();

	virtual void draw2();
};

typedef MainScene* MainScenePtr;