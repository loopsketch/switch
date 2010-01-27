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

struct RemovableMediaArgs {
	const string& driveLetter;
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
	int _luminance;
	bool _startup;
	bool _autoStart;
	string _currentCommand;

	string _playlistID;
	int _playlistItem;

	MediaItemPtr _interruptMedia;

	vector<ContainerPtr> _contents;
	int _currentContent;
	bool _preparing;

	vector<PrepareArgs> _prepareStack;
	int _prepareStackTime;

	/** プレイリスト名 */
	LPDIRECT3DTEXTURE9 _playlistName;
	/** 再生中のコンテンツ名 */
	LPDIRECT3DTEXTURE9 _currentName;
	/** 準備済のプレイリスト名 */
	LPDIRECT3DTEXTURE9 _nextPlaylistName;
	/** 準備済のコンテンツ名 */
	LPDIRECT3DTEXTURE9 _nextName;
	/** 準備済のコマンド */
	string _nextCommand;
	/** 準備済のトランジション */
	string _nextTransition;

	/** 切替準備コンテンツ */
	ContainerPtr _prepared;
	/** 切替準備プレイリスト */
	string _preparedPlaylistID;
	/** 切替準備プレイリストアイテム */
	int _preparedItem;
	/** 切替準備のプレイリスト名 */
	LPDIRECT3DTEXTURE9 _preparedPlaylistName;
	/** 切替準備のコンテンツ名 */
	LPDIRECT3DTEXTURE9 _preparedName;

	/** 再生回数 */
	int _playCount;
	bool _preparingNext;
	/** 切替フラグ */
	bool _doSwitchNext;
	/** 切替準備コンテンツの切替フラグ */
	bool _doSwitchPrepared;
	/** トランジション */
	TransitionPtr _transition;

	/** 現在時刻 */
	string _nowTime;
	/** 現在時刻の秒 */
	int _timeSecond;

	bool _initializing;
	bool _running;

	/** USBアイコン */
	LPDIRECT3DTEXTURE9 _removableIcon;
	string _addRemovable;
	float _removableAlpha;
	float _removableCover;
	unsigned long _copySize;
	unsigned long _currentCopySize;
	int _copyProgress;
	int _currentCopyProgress;


	void run();

	/** 切替用コンテンツの準備 */
	bool prepare(const PrepareArgs& args);

	/** 次再生コンテンツを準備します */
	bool prepareNextMedia();

	/** Containerに指定されたプレイリストのコンテンツを準備します */
	bool prepareMedia(ContainerPtr container, const string& playlistID, const int i = 0);

	void addRemovableMedia(const string& driveLetter);

	int copyFiles(const string& src, const string& dst);

public:
	MainScene(Renderer& renderer, ui::UserInterfaceManager& uim, Path& workspaceFile);

	virtual ~MainScene();

	/** 初期化 */
	bool initialize();

	/** 設定されているWorkspaceを取得します */
	Workspace& getWorkspace();

	void notifyKey(const int keycode, const bool shift, const bool ctrl);

	/** 切替用コンテンツをスタックします */
	bool stackPrepare(string& playlistID, int i = 0);

	/** 切替用コンテンツの準備(アクティブ版) */
	ActiveMethod<bool, PrepareArgs, MainScene> activePrepare;

	/** 手動で切替を行います */
	bool switchContent();

	/** workspace更新 */
	bool updateWorkspace();

	/** 次再生コンテンツを準備(アクティブ版) */
	ActiveMethod<bool, void, MainScene> activePrepareNextMedia;

	/** リムーバブルメディアの追加(アクティブ版) */
	ActiveMethod<void, string, MainScene> activeAddRemovableMedia;

	/** 毎フレームで行う処理 */
	virtual void process();

	/**
	 * フレーム描画
	 * コンテンツなどのメイン描画系
	 */
	virtual void draw1();

	/**
	 * フレーム描画
	 * 主にステータス系
	 */
	virtual void draw2();
};

typedef MainScene* MainScenePtr;
