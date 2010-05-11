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

	/** 再生コンテンツ情報 */
	PlayParameters _playCurrent;

	/** 説明 */
	LPDIRECT3DTEXTURE9 _description;
	/** プレイリスト名 */
	LPDIRECT3DTEXTURE9 _playlistName;
	/** 再生中のコンテンツ名 */
	LPDIRECT3DTEXTURE9 _currentName;

	vector<PlayParameters> _nextStack;
	int _nextStackTime;

	/** 次再生コンテンツ情報 */
	PlayParameters _playNext;

	/** 次のプレイリスト名 */
	LPDIRECT3DTEXTURE9 _nextPlaylistName;
	/** 次のコンテンツ名 */
	LPDIRECT3DTEXTURE9 _nextName;

	vector<PlayParameters> _prepareStack;
	int _prepareStackTime;

	/** 切替準備コンテンツ */
	PlayParameters _playPrepared;

	/** 切替準備コンテンツ情報 */
	ContainerPtr _prepared;

	/** 切替準備のプレイリスト名 */
	LPDIRECT3DTEXTURE9 _preparedPlaylistName;
	/** 切替準備のコンテンツ名 */
	LPDIRECT3DTEXTURE9 _preparedName;

	/** 再生回数 */
	int _playCount;
	bool _doPrepareNext;
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

	/** スタンバイメディア */
	map<string, ContainerPtr> _stanbyMedias;
	ContainerPtr _interrupttedContent;

	/** スタンバイメディアの準備 */
	void preparedStanbyMedia();

	void run();

	/** 切替用コンテンツの準備 */
	bool prepareContent(const PlayParameters& args);

	/** 次再生コンテンツを準備します */
	bool prepareNextContent(const PlayParameters& args);

	/** Containerに指定されたプレイリストのコンテンツを準備します */
	bool preparePlaylist(ContainerPtr container, const string& playlistID, const int i = 0);

	bool prepareMedia(ContainerPtr container, MediaItemPtr media, const string& templatedText);

	void addRemovableMedia(const string& driveLetter);

	int copyFiles(const string& src, const string& dst);

	/** リモートコピー */
	void copyRemote(const string& remote);

	bool copyRemoteFile(const string& remote, const string& path, Path& out, bool equalityCheck = false);

	void setRemoteStatus(const string& remote, const string& name, const string& value);

	/** リモートディレクトリのコピー */
/*	bool copyRemoteDir(const string& remote, const string& root);
*/
	/** 次再生コンテンツを準備(アクティブ版) */
	ActiveMethod<bool, PlayParameters, MainScene> activePrepareNextContent;


public:
	MainScene(Renderer& renderer, ui::UserInterfaceManager& uim, Path& workspaceFile);

	virtual ~MainScene();

	/** コンテンツの遅延解放を行います */
	void delayedReleaseContainer();

	/** 初期化 */
	bool initialize();

	/** 設定されているWorkspaceを取得します */
	Workspace& getWorkspace();

	void notifyKey(const int keycode, const bool shift, const bool ctrl);

	/** 切替用コンテンツをスタックします */
	bool stackPrepareContent(string& playlistID, int i = 0);

	/** 説明設定 */
	const void setDescription(const string& description);

	/** プレイリストテキスト取得 */
	const string getPlaylistText(const string& playlistID);

	/** プレイリストテキスト設定 */
	bool setPlaylistText(const string& playlistID, const string& text);

	/** 輝度設定 */
	void setBrightness(int i);

	/** 遷移アクション設定 */
	void setAction(string& action);

	/** トランジション設定 */
	void setTransition(string& transition);

	/** 切替用コンテンツの準備(アクティブ版) */
	ActiveMethod<bool, PlayParameters, MainScene> activePrepareContent;

	/**
	 * 手動で切替を行います
	 * このメソッドはメインスレッドをブロックするのでメインスレッドからはactiveSwitchContent()で呼出すこと
	 */
	bool switchContent();

	/** 切替用コンテンツに切替(アクティブ版) */
	ActiveMethod<bool, void, MainScene> activeSwitchContent;

	/** 遅延更新ファイルを追加します */
	void addDelayedUpdateFile(File& file);

	/** 遅延更新ファイルを追加します */
	void removeDelayedUpdateFile(File& file);

	/** 遅延更新ファイルを更新します */
	void updateDelayedFiles();

	/** workspace更新 */
	bool updateWorkspace();

	/** リモートコピー (アクティブ版) */
	ActiveMethod<void, string, MainScene> activeCopyRemote;

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
