#pragma once

#include "Scene.h"
#include <d3d9.h>
#include <d3dx9.h>
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
#include "ui/UserInterfaceManager.h"
#include "ui/Button.h"
#include "ui/SelectList.h"
#include "Transition.h"

using std::string;
using std::vector;
using std::queue;


class OperationScene: public Scene
{
private:
	Poco::FastMutex _lock;

	ui::UserInterfaceManager* _uim;

	ui::SelectListPtr _playListSelect;
	ui::SelectListPtr _contentsSelect;
	ui::ButtonPtr _switchButton;

	WorkspacePtr _workspace;

	DWORD _frame;

	MediaItemPtr _interruptMedia;

	vector<ContainerPtr> _contents;
	int _currentContent;
	DWORD _prepareStart;
	bool _prepareUpdate;
	bool _prepared;

	Poco::ActiveMethod<void, void, OperationScene> activeUpdateContentList;
	void updateContentList();

	void updatePrepare();

	Poco::ActiveMethod<void, void, OperationScene> activePrepareContent;
	void prepareContent();

	void switchContent();

public:
	OperationScene(Renderer& renderer, ui::UserInterfaceManagerPtr uim);

	~OperationScene();

	virtual bool initialize();

	virtual bool setWorkspace(WorkspacePtr workspace);

	void prepareInterruptFile(string file);

	virtual void process();

	virtual void draw1();

	virtual void draw2();
};

typedef OperationScene* OperationScenePtr;