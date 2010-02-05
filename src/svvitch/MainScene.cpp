#include "MainScene.h"

#include <Psapi.h>
#include <Poco/format.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Exception.h>
#include <Poco/string.h>
#include <Poco/UnicodeConverter.h>
#include <Poco/Util/XMLConfiguration.h>
#include <Poco/LocalDateTime.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/Timespan.h>
#include <Poco/FileStream.h>
#include <Poco/StreamCopier.h>

#include "Image.h"
#include "FFMovieContent.h"
#include "Text.h"
#include "CvContent.h"
#include "CaptureContent.h"
#include "DSContent.h"
#include "SlideTransition.h"
#include "DissolveTransition.h"
#include "Schedule.h"


MainScene::MainScene(Renderer& renderer, ui::UserInterfaceManager& uim, Path& workspaceFile):
	Scene(renderer), _uim(uim), _workspaceFile(workspaceFile), _workspace(NULL), _updatedWorkspace(NULL),
	activePrepare(this, &MainScene::prepare),
	activePrepareNextMedia(this, &MainScene::prepareNextMedia),
	activeAddRemovableMedia(this, &MainScene::addRemovableMedia),
	_frame(0), _luminance(0), _preparing(false), _playCount(0), _doSwitchNext(false), _doSwitchPrepared(false),
	_transition(NULL), _interruptMedia(NULL),
	_playlistName(NULL), _currentName(NULL), _nextPlaylistName(NULL), _nextName(NULL),
	_prepared(NULL), _preparedPlaylistName(NULL), _preparedName(NULL),
	_initializing(false), _running(false),
	_removableIcon(NULL), _removableAlpha(0), _removableCover(0), _copySize(0), _currentCopySize(0), _copyProgress(0), _currentCopyProgress(0)
{
	initialize();
}

MainScene::~MainScene() {
	_log.information("release contents");
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	for (vector<Container*>::iterator it = _contents.begin(); it != _contents.end(); it++) SAFE_DELETE(*it);
	_contents.clear();

	SAFE_DELETE(_prepared);
	SAFE_DELETE(_transition);
	SAFE_DELETE(_interruptMedia);

	SAFE_RELEASE(_playlistName);
	SAFE_RELEASE(_currentName);
	SAFE_RELEASE(_nextPlaylistName);
	SAFE_RELEASE(_nextName);
	SAFE_RELEASE(_preparedName);
	SAFE_RELEASE(_removableIcon);

	SAFE_DELETE(_workspace);
	_log.information("*release main-scene");
}


bool MainScene::initialize() {
	_contents.clear();
	_contents.push_back(new Container(_renderer));
	_contents.push_back(new Container(_renderer)); // 2個のContainer
	_currentContent = -1;

	LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
	if (!device) return false;
	device->SetRenderState(D3DRS_LIGHTING, false);
	device->SetRenderState(D3DRS_ZENABLE, false);
	device->SetRenderState(D3DRS_ZWRITEENABLE, false);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	device->SetRenderState(D3DRS_ALPHATESTENABLE, false);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
	device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);

	if (true) {
		// blend_nomal
		device->SetRenderState(D3DRS_BLENDOP,   D3DBLENDOP_ADD);
		device->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

	} else {
		// blend_none
		device->SetRenderState(D3DRS_BLENDOP,   D3DBLENDOP_ADD);
		device->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ONE);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	}

	_workspace = new Workspace(_workspaceFile);
	if (!_workspace->parse()) {
		_log.warning("failed parse workspace");
	}
	_timeSecond = -1;
	_frame = 0;
	_luminance = config().luminance;
	_running = true;
	_startup = false;
	_autoStart = false;
	_log.information("*created main-scene");
	return true;
}


Workspace& MainScene::getWorkspace() {
	return *_workspace;
}


void MainScene::notifyKey(const int keycode, const bool shift, const bool ctrl) {
	_keycode = keycode;
	_shift = shift;
	_ctrl = ctrl;
	if (_currentContent >= 0) {
		_contents[_currentContent]->notifyKey(keycode, shift, ctrl);
	}
}

bool MainScene::prepareNextMedia() {
	_preparingNext = true;
	// 初期化フェーズ
	int count = 15;
	while (_transition || count-- > 0) {
		// トランジション中は解放しないようにする。更に初期化まで0.5秒くらいウェイトする
		if (_contents.empty()) {
			_preparingNext = false;
			return false;
		}
		Poco::Thread::sleep(30);
	}

	// 準備フェーズ
	int next = (_currentContent + 1) % _contents.size();
	if (!_action.empty()) {
		int jump = _action.find_first_of("jump");
		if (jump == 0) {
			string s = Poco::trim(_action.substr(4));
			string playlistID;
			int i = s.find("-");
			int j = 0;
			if (i != string::npos) {
				playlistID = s.substr(0, i);
				j = Poco::NumberParser::parse(s.substr(i + 1));
			} else {
				playlistID = s;
			}
			{
				Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
				PlayListPtr playlist = _workspace->getPlaylist(playlistID);
				if (playlist) {
					if (playlist->itemCount() > j) {
						_playlistID = playlist->id();
						_playlistItem = j;
						_log.information(Poco::format("jump playlist <%s>:%d", playlist->name(), _playlistItem));
						_playlistItem--; // 準備時に++されるので引いておく
					} else {
						_log.warning(Poco::format("failed jump index %d-<%d>", i, j));
					}
				} else {
					_log.warning(Poco::format("failed jump index <%d>-%d", i, j));
				}
			}
		} else if (_action == "stop") {
			_contents[next]->initialize();
			_preparingNext = false;
			return true;
		}
	}

	if (prepareMedia(_contents[next], _playlistID, _playlistItem + 1)) {
		string playlistName = "ready";
		string itemName = "ready";
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
			PlayListPtr playlist = _workspace->getPlaylist(_playlistID);
			if (playlist && playlist->itemCount() > 0) {
				playlistName = playlist->name();
				_playlistItem = (_playlistItem + 1) % playlist->itemCount();
				PlayListItemPtr item = playlist->items()[_playlistItem];
				if (item) {
					itemName = item->media()->name();
					_nextAction = item->next();
					_nextTransition = item->transition();
				}
			}
		}
		LPDIRECT3DTEXTURE9 t1 = _renderer.createTexturedText(L"", 14, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, playlistName);
		LPDIRECT3DTEXTURE9 t2 = _renderer.createTexturedText(L"", 14, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, itemName);
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			SAFE_RELEASE(_nextPlaylistName);
			_nextPlaylistName = t1;
			SAFE_RELEASE(_nextName);
			_nextName = t2;
		}
		_status["next-playlist"] = playlistName;
		_status["next-content"] = itemName;
	} else {
		_log.warning(Poco::format("failed prepare: %s-%d", _playlistID, _playlistItem + 1));
	}
	_preparingNext = false;
	return true;
}

bool MainScene::stackPrepare(string& playlistID, int i) {
	ContainerPtr prepared = NULL;
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		if (_prepared) {
			prepared = _prepared;
			_prepared = NULL;
			SAFE_RELEASE(_preparedPlaylistName);
			SAFE_RELEASE(_preparedName);
		}
		PrepareArgs args;
		args.playlistID = playlistID;
		args.i = i;
		_prepareStack.push_back(args);
		if (_prepareStack.size() > 5) _prepareStack.erase(_prepareStack.begin());
		_prepareStackTime = 0;
	}
	std::map<string, string>::iterator it = _status.find("prepared-playlist");
	if (it != _status.end()) _status.erase(it);
	it = _status.find("prepared-content");
	if (it != _status.end()) _status.erase(it);
	SAFE_DELETE(prepared);
	return true;
}

bool MainScene::setPlaylistText(string& playlistID, string& text) {
	Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
	PlayListPtr playlist = _workspace->getPlaylist(playlistID);
	if (playlist) {
		playlist->text(text);
		return true;
	}
	return false;
}

bool MainScene::prepare(const PrepareArgs& args) {
	ContainerPtr c = new Container(_renderer);
	if (prepareMedia(c, args.playlistID, args.i)) {
		string playlistName = "ready";
		string itemName = "ready";
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
			_preparedPlaylistID = args.playlistID;
			_preparedItem = args.i;
			PlayListPtr playlist = _workspace->getPlaylist(_preparedPlaylistID);
			playlistName = playlist->name();
			PlayListItemPtr item = playlist->items()[_preparedItem % playlist->itemCount()];
			itemName = item->media()->name();
			SAFE_DELETE(_prepared);
			_prepared = c;
		}

		LPDIRECT3DTEXTURE9 t1 = _renderer.createTexturedText(L"", 14, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, playlistName);
		LPDIRECT3DTEXTURE9 t2 = _renderer.createTexturedText(L"", 14, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, itemName);
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			SAFE_RELEASE(_preparedPlaylistName);
			_preparedPlaylistName = t1;
			SAFE_RELEASE(_preparedName);
			_preparedName = t2;
		}
		_status["prepared-playlist"] = playlistName;
		_status["prepared-content"] = itemName;
		return true;
	}
	SAFE_DELETE(c);
	return false;
}

bool MainScene::prepareMedia(ContainerPtr container, const string& playlistID, const int i) {
	_log.information(Poco::format("prepare: %s-%d", playlistID, i));
	PlayListPtr playlist = NULL;
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
		playlist = _workspace->getPlaylist(playlistID);
	}
	container->initialize();
	if (playlist && playlist->itemCount() > 0) {
		PlayListItemPtr item = playlist->items()[i % playlist->itemCount()];
		MediaItemPtr media = item->media();
		if (media) {
			_log.information(Poco::format("file: %d", media->fileCount()));
			switch (media->type()) {
				case MediaTypeImage:
					{
						ImagePtr image = new Image(_renderer);
						if (image->open(media)) {
							container->add(image);
						} else {
							SAFE_DELETE(image);
						}
					}
					break;

				case MediaTypeMovie:
					{
						FFMovieContentPtr movie = new FFMovieContent(_renderer);
						// DSContentPtr movie = new DSContent(_renderer);
						if (movie->open(media)) {
							movie->setPosition(config().stageRect.left, config().stageRect.top);
							movie->setBounds(config().stageRect.right, config().stageRect.bottom);
							// movie->set("aspect-mode", "fit");
							container->add(movie);
						} else {
							SAFE_DELETE(movie);
						}
					}
					break;

				case MediaTypeText:
					break;

				case MediaTypeCv:
					{
						CvContentPtr cv = new CvContent(_renderer);
						if (cv->open(media)) {
							container->add(cv);
						} else {
							SAFE_DELETE(cv);
						}
					}
					break;

				case MediaTypeCvCap:
					{
						CaptureContentPtr cvcap = new CaptureContent(_renderer);
						if (cvcap->open(media)) {
							container->add(cvcap);
						} else {
							SAFE_DELETE(cvcap);
						}
					}
					break;

				default:
					_log.warning("media type: unknown");
			}
			if (media->containsFileType(MediaTypeText)) {
				_log.information("contains text");
				TextPtr ref = NULL;
				for (int j = 0; j < media->fileCount(); j++) {
					MediaItemFile mif = media->files().at(j);
					if (mif.type() == MediaTypeText) {
						TextPtr text = new Text(_renderer);
						if (text->open(media, j)) {
							if (ref) {
								text->setReference(ref);
							} else {
								if (mif.file().empty()) {
									_log.information(Poco::format("tempate text: %s", playlist->text()));
									text->drawTexture(playlist->text());
									if (!ref) ref = text;
								}
							}
							container->add(text);
						} else {
							SAFE_DELETE(text);
						}
					}
				}
			}
			if (container->size() > 0) {
				_log.information(Poco::format("prepared: %s", media->name()));
				return true;
			} else {
				_log.warning("failed prepare next media");
			}
		} else {
			_log.warning("failed prepare next media, no media item");
		}
	} else {
		_log.warning("failed prepare next media, no item in current playlist");
	}
	return false;
}

bool MainScene::switchContent() {
	_doSwitchPrepared = true;
	while (_doSwitchPrepared) {
		Poco::Thread::sleep(30);
	}
//	_log.information("purge prepared next contents");
//	SAFE_DELETE(_prepared);
	return true;
}

bool MainScene::updateWorkspace() {
	_log.information("update workspace");
	if (_workspace->checkUpdate()) {
		WorkspacePtr workspace = new Workspace(_workspace->file());
		if (workspace->parse()) {
			_log.information("updated workspace. repreparing next contents");
			_updatedWorkspace = workspace;
			return true;
		} else {
			_log.warning("failed update workspace.");
			SAFE_DELETE(workspace);
		}
	} else {
		_log.information("there is no need for updates.");
		return true;
	}
	return false;
}

void MainScene::addRemovableMedia(const string& driveLetter) {
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		if (!_addRemovable.empty() && _addRemovable == driveLetter) {
			_log.information(Poco::format("processing same device, ignore notifier removable device[%s]", driveLetter));
			return;
		}
		_addRemovable = driveLetter;
	}
	_log.information(Poco::format("addRemovableMedia: %s", driveLetter));
	Sleep(2000);

	if (!_removableIcon) {
		LPDIRECT3DTEXTURE9 texture = _renderer.createTexture("images/Crystal_Clear_device_usbpendrive_unmount.png");
		if (texture) {
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			_removableIcon = texture;
		}
	}
	_copySize = 0;
	_copyProgress = 0;
	_currentCopyProgress = 0;
	_removableAlpha = 0.01f;
	_removableCover = 0;

	Path dst = Path(config().dataRoot.parent(), "removable-copys\\");
	File dir(dst);
	if (dir.exists()) dir.remove(true);
	int size = copyFiles(Poco::format("%s:\\switch-datas", driveLetter), "");
	_currentCopySize = 0;
	_copySize = size;
	copyFiles(Poco::format("%s:\\switch-datas", driveLetter), dst.toString());
	_renderer.ejectVolume(driveLetter);

	// Workspaceを仮生成
	WorkspacePtr workspace = new Workspace(Path(dst, "workspace.xml"));
	if (!workspace->parse()) {
		SAFE_DELETE(workspace);
		_removableAlpha = 0;
		_addRemovable.clear();
		return;
	}
	SAFE_DELETE(workspace);
	while (_currentCopyProgress < 100) {
//		_log.information(Poco::format("size:%lu/%lu progress:%d%%", _currentCopySize, _copySize, _copyProgress));
		Sleep(100);
	}

	if (_startup) {
		_log.information("wait viewing removable cover");
		while (_removableCover < 1.0f) {
			Sleep(100);
		}
		_log.information("stop current playing");
		_running = false;
		Sleep(2000);
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			for (vector<Container*>::iterator it = _contents.begin(); it != _contents.end(); it++) {
				(*it)->initialize();
			}
		}
		_log.information("purge all contents");
		SAFE_RELEASE(_playlistName);
		SAFE_RELEASE(_currentName);
		SAFE_RELEASE(_nextPlaylistName);
		SAFE_RELEASE(_nextName);
		_status.clear();
		_action.clear();
		_nextTransition.clear();
	} else {
		_running = false;
		while (_removableAlpha > 0.0f) {
			Sleep(100);
		}
	}
	_running = true;
	_copySize = 0;
	_removableCover = 0.0f;

	_log.information("backup current data");
	File old(Path(config().dataRoot.parent(), "datas_old"));
	if (old.exists()) old.remove(true);
	File currentDataDir(config().dataRoot);
	if (currentDataDir.exists()) {
		currentDataDir.renameTo(old.path());
	}
	dir.renameTo(config().dataRoot.toString());
	if (updateWorkspace()) {
		_startup = false;
	}
	_addRemovable.clear();
}

int MainScene::copyFiles(const string& src, const string& dst) {
	int size = 0;
	Poco::DirectoryIterator it(src);
	Poco::DirectoryIterator end;
	while (it != end) {
		if (it->isDirectory()) {
			File dir(dst + it.path().getFileName() + "\\");
			if (!dir.exists()) dir.createDirectories();
			if (dst.empty()) {
				size += copyFiles(it->path(), "");
			} else {
				size += copyFiles(it->path(), dst + it.path().getFileName() + "\\");
			}
		} else {
			size += it->getSize();
			if (!dst.empty()) {
				_log.information(Poco::format("copy: %s -> %s", it->path(), dst + it.path().getFileName()));
				try {
					File dir(dst);
					if (!dir.exists()) dir.createDirectories();
					Poco::FileInputStream is(it->path());
					Poco::FileOutputStream os(dst + it.path().getFileName());
					if (is.good() && os.good()) {
						Poco::StreamCopier::copyStream(is, os);
						_currentCopySize += it->getSize();
					}
				} catch (Poco::FileException ex) {
					_log.warning(ex.displayText());
				}
			}
		}
		++it;
	}
	return size;
}


void MainScene::process() {
	switch (_keycode) {
		case 'Z':
			if (config().luminance > 0) config().luminance--;
			break;
		case 'X':
			if (config().luminance < 100) config().luminance++;
			break;
	}
	if (_luminance < config().luminance) {
		_luminance++;
	} else if (_luminance > config().luminance) {
		_luminance--;
	}

	// リムーバブルメディア検出
	string drive = _renderer.popReadyDrive();
	if (!drive.empty()) {
		activeAddRemovableMedia(drive);
		//_log.information(Poco::format("volume device arrival: %s",drive));
	}

	if (_running && _removableAlpha > 0) {
		_removableAlpha += 0.01f;
		if (_removableAlpha >= 1.0f) _removableAlpha = 1;
	}
	if (_copySize > 0) {
		_copyProgress = 100 * F(_currentCopySize) / F(_copySize);
		if (_currentCopyProgress < _copyProgress) _currentCopyProgress++;
		if (_copyProgress >= 100 && _removableCover < 1.0f) {
			_removableCover += 0.01f;
			if (_removableCover > 1.0f) _removableCover = 1.0f;
		}
		if (!_running && _removableAlpha > 0.0f) {
			_removableAlpha -= 0.01f;
			if (_removableAlpha < 0.0f) _removableAlpha = 0.0f;
		}
	}

	// ワークスペースの更新チェック
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		if (_updatedWorkspace && !_preparingNext && _prepareStack.empty()) {
			Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
			SAFE_DELETE(_workspace);
			_workspace = _updatedWorkspace;
			_updatedWorkspace = NULL;
			if (!_status["next-content"].empty() && _action != "stop") {
				_playlistItem--;
				activePrepareNextMedia();
			}
		}
	}

	if (!_running) return;

	if (!_startup && _frame > 100) {
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
			if (_workspace->getPlaylistCount() > 0) {
				// playlistがある場合は最初のplaylistを自動スタートする
				PlayListPtr playlist = _workspace->getPlaylist(0);
				if (playlist) {
					_playlistID = playlist->id();
					_playlistItem = -1;
					activePrepareNextMedia();
					_autoStart = true;
					_frame = 0;
					_startup = true;
				}
			} else {
				if (_frame == 101) _log.warning("no playlist, no auto starting");
				// _frame = 0;
			}
		}
	} else if (_startup) {
		_status["action"] = _action;
		_status["transition"] = _nextTransition;
		for (vector<Container*>::iterator it = _contents.begin(); it != _contents.end(); it++) {
			(*it)->process(_frame);
		}

		ContentPtr currentContent = NULL;
		if (_currentContent >= 0) currentContent = _contents[_currentContent]->get(0);

		if (currentContent) {
			if (_action == "stop") {

			} else if (_action == "wait-prepared") {
				if (!_status["next-content"].empty()) {
					// _log.information("wait prepared next content, prepared now.");
					_doSwitchNext = true;
				}
			} else {
				// コマンド指定が無ければ現在再生中のコンテンツの終了を待つ
				if (_contents[_currentContent]->finished()) {
					// _log.information(Poco::format("content[%d] finished: ", _currentContent));
					_doSwitchNext = true;
				}
			}
		} else {
			if (_autoStart && !_status["next-content"].empty() && _frame > 200) {
				_log.information("auto start content");
				_doSwitchNext = true;
				_autoStart = false;
			}
		}

		if (_doSwitchPrepared) {
			if (_prepared) {
				int next = (_currentContent + 1) % _contents.size();
				ContainerPtr tmp = _contents[next];
				_contents[next] = _prepared;
				_prepared = tmp;
				{
					Poco::ScopedLock<Poco::FastMutex> lock(_lock);
					LPDIRECT3DTEXTURE9 tmp = _nextPlaylistName;
					_nextPlaylistName = _preparedPlaylistName;
					_preparedPlaylistName = tmp;
					tmp = _nextName;
					_nextName = _preparedName;
					_preparedName = tmp;
				}
				Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
				PlayListPtr playlist = _workspace->getPlaylist(_preparedPlaylistID);
				if (_prepared && playlist && playlist->itemCount() > _preparedItem) {
					PlayListItemPtr item = playlist->items()[_preparedItem];
					if (item) {
						_log.information(Poco::format("switch content: %s-%d: %s", _preparedPlaylistID, _preparedItem, item->media()->name()));
						_playlistID = _preparedPlaylistID;
						_playlistItem = _preparedItem;
						_nextAction = item->next();
						_nextTransition = item->transition();
					}
				} else {
					_log.warning(Poco::format("not find playlist: %s", _preparedPlaylistID));
				}
				_status["next-playlist"] = _status["prepared-playlist"];
				_status["next-content"] = _status["prepared-content"];
				_doSwitchNext = true;
				{
					Poco::ScopedLock<Poco::FastMutex> lock(_lock);
					SAFE_RELEASE(_preparedPlaylistName);
					SAFE_RELEASE(_preparedName);
				}
				{
					Poco::ScopedLock<Poco::FastMutex> lock(_lock);
					SAFE_DELETE(_transition);
				}
				//std::map<string, string>::iterator it = _status.find("prepared-playlist");
				//if (it != _status.end()) _status.erase(it);
				//it = _status.find("prepared-content");
				//if (it != _status.end()) _status.erase(it);
				_status.erase(_status.find("prepared-playlist"));
				_status.erase(_status.find("prepared-content"));

			} else {
				// 準備済みコンテンツが無い場合は次のコンテンツへ切替
				int next = (_currentContent + 1) % _contents.size();
				ContainerPtr tmp = _contents[next];
				if (tmp && tmp->size() > 0) {
					_log.information("switch next contents");
					{
						Poco::ScopedLock<Poco::FastMutex> lock(_lock);
						SAFE_DELETE(_transition);
					}
					_doSwitchNext = true;
				}
			}
			_doSwitchPrepared = false;
		}

		bool prepareNext = false;
		if (_doSwitchNext && !_transition) {
			_doSwitchNext = false;
			int next = (_currentContent + 1) % _contents.size();
			ContentPtr nextContent = _contents[next]->get(0);
			if (nextContent && !nextContent->opened().empty()) {
				_currentContent = next;
				_contents[next]->play();
				{
					Poco::ScopedLock<Poco::FastMutex> lock(_lock);
					SAFE_RELEASE(_playlistName);
					_playlistName = _nextPlaylistName;
					_nextPlaylistName = NULL;
					SAFE_RELEASE(_currentName);
					_currentName = _nextName;
					_nextName = NULL;
				}
				_status["current-playlist"] = _status["next-playlist"];
				_status["current-content"] = _status["next-content"];
				_status.erase(_status.find("next-playlist"));
				_status.erase(_status.find("next-content"));
				_action = _nextAction;
				_playCount++;

//				if (_transition) SAFE_DELETE(_transition);
				if (currentContent) {
					Poco::ScopedLock<Poco::FastMutex> lock(_lock);
					if (_nextTransition == "slide") {
						int cw = config().splitSize.cx;
						int ch = config().splitSize.cy;
						_transition = new SlideTransition(currentContent, nextContent, 0, ch);
					} else if (_nextTransition == "dissolve") {
						_transition = new DissolveTransition(currentContent, nextContent);
					}
					if (_transition) _transition->initialize(_frame);
				}
				if (!_transition) {
					prepareNext = true;
				}
				} else {
				//
			}
		}

		if (_transition) {
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			if (_transition && _transition->process(_frame)) {
				// トランジション終了
				SAFE_DELETE(_transition);
				// 次のコンテンツ準備
				prepareNext = true;
			}
		}
		if (prepareNext) {
			try {
				activePrepareNextMedia();
			} catch (Poco::NoThreadAvailableException ex) {
			}
		}
	}

	{
		// プレイリスト切替のスタック処理
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		if (!_prepareStack.empty()) {
			_prepareStackTime++;
			if (_prepareStackTime > 15) {
				activePrepare(_prepareStack.back());
				_prepareStack.clear();
			}
		}
	}

	// スケジュール処理
	Poco::LocalDateTime now;
	if (now.second() != _timeSecond) {
		Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
		_timeSecond = now.second();
		_nowTime = Poco::DateTimeFormatter::format(now, "%Y/%m/%d(%w) %H:%M:%S");
		Poco::Timespan span(0, 0, 0, 10, 0);
		for (int i = 0; i < _workspace->getScheduleCount(); i++) {
			SchedulePtr schedule = _workspace->getSchedule(i);
			if (schedule->check(now + span)) {
				// 10秒前チェック
				string command = schedule->command();
				if (command.find("playlist ") == 0) {
					string playlistID = command.substr(9);
					PlayListPtr playlist = _workspace->getPlaylist(playlistID);
					if (playlist) {
						_log.information(Poco::format("[%s]exec %s", _nowTime, command));
						stackPrepare(playlistID);
					} else {
						_log.warning(Poco::format("[%s]failed command: %s", _nowTime, command));
					}
					break;
				} else {
					_log.warning(Poco::format("[%s]failed command: %s", _nowTime, command));
				}
			} else if (schedule->check(now)) {
				// 実時間チェック
				string command = schedule->command();
				if (command.find("playlist ") == 0) {
					if (_prepared) {
						_log.information(Poco::format("[%s]exec %s", _nowTime, command));
						_doSwitchPrepared = true;
					} else {
						_log.warning(Poco::format("[%s]failed next content not prepared %s", _nowTime, command));
					}
					break;
				}
			}
		}
	}

	_status["luminance"] = Poco::format("%d", config().luminance);
	_frame++;
}

void MainScene::draw1() {
	if (!_running) return;
	LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
	device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	if (_currentContent >= 0) {
		int before = _currentContent - 1;
		if (before < 0) before = _contents.size() - 1;
		_contents[before]->draw(_frame);
		_contents[_currentContent]->draw(_frame);
	}

	if (_luminance < 100) {
		DWORD col = ((DWORD)(0xff * (100 - _luminance) / 100) << 24) | 0x000000;
		_renderer.drawTexture(config().mainRect.left, config().mainRect.top, config().mainRect.right, config().mainRect.bottom, NULL, 0, col, col, col, col);
	}
	if (_removableCover > 0.0f) {
		DWORD col = ((DWORD)(0xff * _removableCover) << 24) | 0x000000;
		_renderer.drawTexture(config().mainRect.left, config().mainRect.top, config().mainRect.right, config().mainRect.bottom, NULL, 0, col, col, col, col);
	}
	// _renderer.drawFontTextureText(0, config().mainRect.bottom - 40, 12, 16, 0xffcccccc, Poco::format("LUMINANCE:%03d", _luminance));
}

void MainScene::draw2() {
	if (_renderer.getDisplayAdapters() > 1) {
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		LPDIRECT3DTEXTURE9 capture = _renderer.getCaptureTexture();
		_renderer.drawTexture(0, 50, 256, 192, capture, 0, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	}

	string status1;
	string status2;
	if (_currentContent >= 0) {
		ContentPtr c = _contents[_currentContent]->get(0);
		if (c && !c->opened().empty()) {
			int current = c->current();
			int duration = c->duration();
			string time;
			string buffers;
			FFMovieContentPtr movie = dynamic_cast<FFMovieContentPtr>(c);
			if (movie != NULL) {
				Uint32 fps = movie->getFPS();
				float avgTime = movie->getAvgTime();
				status1 = Poco::format("%03lufps(%03.2hfms)", fps, avgTime);
				buffers = movie->get("buffers");
			}
			time = c->get("time");
			_status["time_remain"] = c->get("time_remain");
			status2 = Poco::format("%04d/%04d %s %s", current, duration, time, buffers);
		}
	}

	if (config().viewStatus) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		_renderer.drawFontTextureText(0, 640, 12, 16, 0xccffffff, status1);
		_renderer.drawFontTextureText(0, 660, 12, 16, 0xccffffff, status2);
		if (!_action.empty()) _renderer.drawFontTextureText(0, 680, 12, 16, 0xccffffff, Poco::format("action>%s", _action));
		if (!_nextTransition.empty()) _renderer.drawFontTextureText(0, 700, 12, 16, 0xccffffff, Poco::format("transition>%s", _nextTransition));

		_renderer.drawTexture(700, 640, _playlistName, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
		_renderer.drawTexture(700, 655, _currentName, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
		_renderer.drawTexture(700, 670, _nextName, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
		_renderer.drawTexture(700, 685, _preparedPlaylistName, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
		_renderer.drawTexture(700, 700, _preparedName, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);

		int next = (_currentContent + 1) % _contents.size();
		string wait(_contents[next]->opened().empty()?"preparing":"ready");
		_renderer.drawFontTextureText(0, 730, 12, 16, 0xccffffff, Poco::format("[%s] play contents:%04d playing no<%d> next:%s", _nowTime, _playCount, _currentContent, wait));
	}

	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		if (_removableIcon && _removableAlpha > 0) {
			D3DSURFACE_DESC desc;
			HRESULT hr = _removableIcon->GetLevelDesc(0, &desc);
			int tw = desc.Width / 2;
			int th = desc.Height / 2;
			DWORD col = ((DWORD)(0xff * _removableAlpha) << 24) | 0xffffff;
			_renderer.drawTexture(0, config().subRect.bottom - th, tw, th, _removableIcon, 0, col, col, col, col);
			col = ((DWORD)(0x66 * _removableAlpha) << 24) | 0x333333;
			_renderer.drawTexture(tw + 1, config().subRect.bottom - th / 2, config().subRect.right - tw - 1, 10, NULL, 0, col, col, col, col);
			DWORD col1 = ((DWORD)(0x66 * _removableAlpha) << 24) | 0x33ccff;
			DWORD col2 = ((DWORD)(0x66 * _removableAlpha) << 24) | 0x3399cc;
			_renderer.drawTexture(tw + 2, config().subRect.bottom - th / 2 + 1, (config().subRect.right - tw - 2) * _currentCopyProgress / 100, 8, NULL, 0, col1, col1, col2, col2);
//			_renderer.drawFontTextureText(tw, config().subRect.bottom - th / 2, 12, 16, 0xccffffff, Poco::format("%d %d(%d%%)", _currentCopySize, _copySize, _currentCopyProgress));
		}
	}
}