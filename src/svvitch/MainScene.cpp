#include "MainScene.h"

#include <Psapi.h>
#include <Poco/format.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeParser.h>
#include <Poco/Exception.h>
#include <Poco/string.h>
#include <Poco/NumberFormatter.h>
#include <Poco/UnicodeConverter.h>
#include <Poco/Util/XMLConfiguration.h>
#include <Poco/LocalDateTime.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/Timespan.h>
#include <Poco/Timezone.h>
#include <Poco/FileStream.h>
#include "Poco/URIStreamOpener.h"
#include "Poco/URI.h"

#include "Image.h"
#include "FFMovieContent.h"
#include "Text.h"
#include "CvContent.h"
#include "CaptureContent.h"
#include "DSContent.h"
#include "SlideTransition.h"
#include "DissolveTransition.h"
#include "Schedule.h"

#include "Utils.h"


MainScene::MainScene(Renderer& renderer, ui::UserInterfaceManager& uim, Path& workspaceFile):
	Scene(renderer), _uim(uim), _workspaceFile(workspaceFile), _workspace(NULL), _updatedWorkspace(NULL),
	activePrepareContent(this, &MainScene::prepareContent),
	activePrepareNextContent(this, &MainScene::prepareNextContent),
	activeSwitchContent(this, &MainScene::switchContent),
	activeCopyRemote(this, &MainScene::copyRemote),
	activeAddRemovableMedia(this, &MainScene::addRemovableMedia),
	_frame(0), _brightness(100), _preparing(false), _playCount(0), _doPrepareNext(false), _preparingNext(false), _doSwitchNext(false), _doSwitchPrepared(false),
	_transition(NULL),
	_playlistName(NULL), _currentName(NULL), _nextPlaylistName(NULL), _nextName(NULL),
	_prepared(NULL), _preparedPlaylistName(NULL), _preparedName(NULL),
	_initializing(false), _running(false),
	_removableIcon(NULL), _removableIconAlpha(0), _removableAlpha(0), _removableCover(0), _copySize(0), _currentCopySize(0), _copyProgress(0), _currentCopyProgress(0),
	_copyRemoteFiles(0), _interrupttedContent(NULL)
{
	initialize();
}

MainScene::~MainScene() {
	_log.information("release contents");
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	for (vector<Container*>::iterator it = _contents.begin(); it != _contents.end(); it = _contents.erase(it)) {
		SAFE_DELETE(*it);
	}

	delayedReleaseContainer();
	SAFE_DELETE(_prepared);
	SAFE_DELETE(_transition);

	SAFE_RELEASE(_playlistName);
	SAFE_RELEASE(_currentName);
	SAFE_RELEASE(_nextPlaylistName);
	SAFE_RELEASE(_nextName);
	SAFE_RELEASE(_preparedName);
	SAFE_RELEASE(_removableIcon);

	SAFE_DELETE(_workspace);
	preparedStanbyMedia();
	_log.information("*release main-scene");
}

void MainScene::delayedReleaseContainer() {
	int count = 0;
	for (vector<ContainerPtr>::iterator it = _delayReleases.begin(); it != _delayReleases.end(); ) {
		SAFE_DELETE(*it);
		it = _delayReleases.erase(it);
		count++;
	}
	_log.information(Poco::format("delayed release: %d", count));
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

	LPDIRECT3DTEXTURE9 texture = _renderer.createTexture("images/Crystal_Clear_device_usbpendrive_unmount.png");
	if (texture) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		_removableIcon = texture;
	}

	_workspace = new Workspace(_workspaceFile);
	if (_workspace->parse()) {
		preparedStanbyMedia();
	} else {
		_log.warning("failed parse workspace");
	}
	_timeSecond = -1;
	_frame = 0;
	_brightness = config().brightness;
	_running = true;
	_startup = false;
	_autoStart = false;
	_log.information("*created main-scene");
	return true;
}

void MainScene::preparedStanbyMedia() {
	Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
	for (map<string, ContainerPtr>::iterator it = _stanbyMedias.begin(); it != _stanbyMedias.end(); it = _stanbyMedias.erase(it)) {
		SAFE_DELETE(it->second);
	}
	if (_workspace) {
		for (int i = 0; i < _workspace->getMediaCount(); i++) {
			MediaItemPtr media = _workspace->getMedia(i);
			if (media->stanby()) {
				ContainerPtr c = new Container(_renderer);
				if (prepareMedia(c, media, "")) {
					_stanbyMedias[media->id()] = c;
					_log.information(Poco::format("standby media: %s", media->id()));
				}
			}
		}
	}
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

bool MainScene::prepareNextContent(const PlayParameters& params) {
	_preparingNext = true;
	ContainerPtr c = new Container(_renderer);
	ContainerPtr oldNextContainer = NULL;
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		int next = (_currentContent + 1) % _contents.size();
		oldNextContainer = _contents[next];
		_contents[next] = c;
	}
	SAFE_DELETE(oldNextContainer);

	string playlistID = params.playlistID;
	int i = params.i;

	_log.information(Poco::format("prepare next content: %s-%d %s", playlistID, i, params.action));
	if (!params.action.empty()) {
		int jump = params.action.find_first_of("jump");
		if (jump == 0) {
			string s = Poco::trim(params.action.substr(4));
			int j = s.find("-");
			if (j != string::npos) {
				playlistID = s.substr(0, j);
				i = Poco::NumberParser::parse(s.substr(j + 1));
			} else {
				playlistID = s;
				i = 0;
			}
		} else if (params.action == "stop") {
			//int next = (_currentContent + 1) % _contents.size();
			//_contents[next]->initialize();
			_preparingNext = false;
			LPDIRECT3DTEXTURE9 oldNextPlaylistName = NULL;
			LPDIRECT3DTEXTURE9 oldNextName = NULL;
			{
				Poco::ScopedLock<Poco::FastMutex> lock(_lock);
				oldNextPlaylistName = _nextPlaylistName;
				_nextPlaylistName = NULL;
				oldNextName = _nextName;
				_nextName = NULL;
			}
			SAFE_RELEASE(oldNextPlaylistName);
			SAFE_RELEASE(oldNextName);
			removeStatus("next-playlist");
			removeStatus("next-content-id");
			removeStatus("next-content");
			return true;
		}
	}

	c = new Container(_renderer);
	if (preparePlaylist(c, playlistID, i)) {
		string playlistName = "ready";
		string itemID = "";
		string itemName = "ready";
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
			PlayListPtr playlist = _workspace->getPlaylist(playlistID);
			if (playlist && playlist->itemCount() > 0) {
				playlistName = playlist->name();
				i = i % playlist->itemCount();
				PlayListItemPtr item = playlist->items().at(i);
				if (item) {
					_playNext.playlistID = playlistID;
					_playNext.i = i;
					_playNext.action = item->next();
					_playNext.transition = item->transition();
					itemID = item->media()->id();
					itemName = item->media()->name();
				}
			}
		}
		LPDIRECT3DTEXTURE9 t1 = _renderer.createTexturedText(L"", 14, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, playlistName);
		LPDIRECT3DTEXTURE9 t2 = _renderer.createTexturedText(L"", 14, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, itemName);
		ContainerPtr oldNextContainer = NULL;
		LPDIRECT3DTEXTURE9 oldNextPlaylistName = NULL;
		LPDIRECT3DTEXTURE9 oldNextName = NULL;
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			int next = (_currentContent + 1) % _contents.size();
			oldNextContainer = _contents[next];
			_contents[next] = c;
			oldNextPlaylistName = _nextPlaylistName;
			_nextPlaylistName = t1;
			oldNextName = _nextName;
			_nextName = t2;
		}
		SAFE_DELETE(oldNextContainer);
		SAFE_RELEASE(oldNextPlaylistName);
		SAFE_RELEASE(oldNextName);
		_status["next-playlist"] = playlistName;
		_status["next-content-id"] = itemID;
		_status["next-content"] = itemName;
	} else {
		SAFE_DELETE(c);
		_log.warning(Poco::format("failed prepare: %s-%d", playlistID, i));
	}
	_preparingNext = false;
	return true;
}

bool MainScene::stackPrepareContent(string& playlistID, int i) {
	PlayParameters args;
	args.playlistID = playlistID;
	args.i = i;
	_prepareStack.push_back(args);
	if (_prepareStack.size() > 5) _prepareStack.erase(_prepareStack.begin());
	_prepareStackTime = 0;
	return true;
}

const string MainScene::getPlaylistText(const string& playlistID) {
	Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
	PlayListPtr playlist = _workspace->getPlaylist(playlistID);
	if (playlist) {
		return playlist->text();
	}
	return "";
}

bool MainScene::setPlaylistText(const string& playlistID, const string& text) {
	Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
	PlayListPtr playlist = _workspace->getPlaylist(playlistID);
	if (playlist) {
		playlist->text(text);
		// _status["set-text"] = Poco::format("%s:%s", playlistID, text);
		return true;
	}
	return false;
}

void MainScene::setBrightness(int i) {
	config().brightness = i;
}

void MainScene::setAction(string& action) {
	_playCurrent.action = action;
}

void MainScene::setTransition(string& transition) {
	_playNext.transition = transition;
}

bool MainScene::prepareContent(const PlayParameters& params) {
	removeStatus("prepared-playlist");
	removeStatus("prepared-content");
	ContainerPtr oldPrepared = NULL;
	LPDIRECT3DTEXTURE9 oldPreparedPlaylistName = NULL;
	LPDIRECT3DTEXTURE9 oldPreparedName = NULL;
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		oldPrepared = _prepared;
		_prepared = NULL;
		oldPreparedPlaylistName = _preparedPlaylistName;
		_preparedPlaylistName = NULL;
		oldPreparedName = _preparedName;
		_preparedName = NULL;
	}
	SAFE_DELETE(oldPrepared);
	SAFE_RELEASE(oldPreparedPlaylistName);
	SAFE_RELEASE(oldPreparedName);

	ContainerPtr c = new Container(_renderer);
	if (preparePlaylist(c, params.playlistID, params.i)) {
		string playlistName = "ready";
		string itemID = "";
		string itemName = "ready";
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
			PlayListPtr playlist = _workspace->getPlaylist(params.playlistID);
			playlistName = playlist->name();
			PlayListItemPtr item = playlist->items()[params.i % playlist->itemCount()];
			itemID = item->media()->id();
			itemName = item->media()->name();
			_playPrepared.playlistID = params.playlistID;
			_playPrepared.i = params.i;
			_playPrepared.action = item->next();
			_playPrepared.transition = item->transition();
		}

		LPDIRECT3DTEXTURE9 t1 = _renderer.createTexturedText(L"", 14, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, playlistName);
		LPDIRECT3DTEXTURE9 t2 = _renderer.createTexturedText(L"", 14, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, itemName);
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			_prepared = c;
			_preparedPlaylistName = t1;
			_preparedName = t2;
		}
		_status["prepared-playlist"] = playlistName;
		_status["prepared-content-id"] = itemID;
		_status["prepared-content"] = itemName;
		return true;
	}
	SAFE_DELETE(c);
	return false;
}

bool MainScene::preparePlaylist(ContainerPtr container, const string& playlistID, const int listIndex) {
	_log.information(Poco::format("prepare: %s-%d", playlistID, listIndex));
	PlayListPtr playlist = NULL;
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
		playlist = _workspace->getPlaylist(playlistID);
	}
	container->initialize();
	if (playlist && playlist->itemCount() > 0) {
		PlayListItemPtr item = playlist->items()[listIndex % playlist->itemCount()];
		MediaItemPtr media = item->media();
		if (media) {
			return prepareMedia(container, media, playlist->text());
		} else {
			_log.warning("failed prepare next media, no media item");
		}
	} else {
		_log.warning("failed prepare next media, no item in current playlist");
	}
	return false;
}

bool MainScene::prepareMedia(ContainerPtr container, MediaItemPtr media, const string& templatedText) {
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
		vector<TextPtr> ref;
		int j = 0;
		int block = -1;
		for (int i = 0; i < media->fileCount(); i++) {
			MediaItemFile mif = media->files().at(i);
			if (mif.type() == MediaTypeText) {
				TextPtr text = new Text(_renderer);
				if (text->open(media, i)) {
					if (block <= 0) {
						// 実体モード
						if (mif.file().empty()) {
							_log.information(Poco::format("tempate text: %s", templatedText));
							text->drawTexture(templatedText);
						} else if (mif.file().find("$") != string::npos) {
							int line = 0;
							if (Poco::NumberParser::tryParse(mif.file().substr(1), line)) {
								string s = Poco::trim(templatedText);
								if (line > 0) {
									vector<string> texts;
									svvitch::split(s, '\r', texts);
									if (texts.size() >= line) {
										s = texts[line - 1];
										text->drawTexture(s);
									} else {
										_log.warning(Poco::format("not enough lines: %u", texts.size()));
									}
								} else {
									text->drawTexture(s);
								}
							} else {
								_log.warning(Poco::format("failed text URL: %s", mif.file()));
							}
						}
						ref.push_back(text);
					} else {
						// 参照モード
						text->setReference(ref.at(j++ % ref.size()));
					}
					container->add(text);
				} else {
					SAFE_DELETE(text);
				}
			} else {
				block++;
				j = 0;
			}
		}
	}
	if (container->size() > 0) {
		_log.information(Poco::format("prepared: %s", media->name()));
		return true;
	} else {
		_log.warning("failed prepare next media");
	}
	return false;
}

bool MainScene::switchContent() {
	_doSwitchPrepared = true;
	while (_doSwitchPrepared) {
		Poco::Thread::sleep(30);
	}
	delayedReleaseContainer();
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

/** リモートコピー */
void MainScene::copyRemote(const string& remote) {
	_log.information(Poco::format("remote copy: %s", remote));
	File copyDir("copys");
	if (copyDir.exists()) copyDir.remove(true);
	bool result = copyRemoteDir(remote, "/");
}

bool MainScene::copyRemoteDir(const string& remote, const string& root) {
	_log.information(Poco::format("remote copy directory: %s", root));
	try {
		Poco::URI uri(Poco::format("http://%s/files?path=%s", remote, root));
		std::auto_ptr<std::istream> is(Poco::URIStreamOpener::defaultOpener().open(uri));
		string result;
		Poco::StreamCopier::copyToString(*is.get(), result);
		//_log.information(Poco::format("result: %s", result));
		map<string, string> m;
		svvitch::parseJSON(result, m);
		//for (map<string, string>::iterator it = m.begin(); it != m.end(); it++) {
		//	_log.information(Poco::format("[%s]=%s", it->first, it->second));
		//}
		if (root == "/") {
			Poco::NumberParser::tryParse(m["count"], _copyRemoteFiles);
			_log.information(Poco::format("copy files: %d", _copyRemoteFiles));
		}
		vector<string> v;
		svvitch::parseJSONArray(m["files"], v);
		File dataDir("datas");
		File copyDir("copys");
		if (!copyDir.exists()) copyDir.createDirectories();
		for (vector<string>::iterator it = v.begin(); it != v.end(); it++) {
			string remotePath = root + *it;
			if (remotePath.at(remotePath.length() - 1) == '/') {
				copyRemoteDir(remote, remotePath);

			} else {
				Poco::DateTime modified;
				int tzd = Poco::Timezone::tzd();
				modified.makeLocal(tzd);
				try {
					Poco::URI uri(Poco::format("http://%s/files?path=%s", remote, remotePath));
					std::auto_ptr<std::istream> is(Poco::URIStreamOpener::defaultOpener().open(uri));
					string result;
					Poco::StreamCopier::copyToString(*is.get(), result);
					_log.information(Poco::format("result: %s", result));
					map<string, string> m;
					svvitch::parseJSON(result, m);
					svvitch::parseJSON(m["files"], m);
					for (map<string, string>::iterator it = m.begin(); it != m.end(); it++) {
						_log.information(Poco::format("[%s]=%s", it->first, it->second));
					}
					int tz = 0;
					Poco::DateTimeParser::parse(Poco::DateTimeFormat::SORTABLE_FORMAT, m["modified"], modified, tz);
					modified.makeUTC(tzd);
					_log.information(Poco::format("modified: %s", Poco::DateTimeFormatter::format(modified, Poco::DateTimeFormat::SORTABLE_FORMAT)));

				} catch (Poco::FileException& ex) {
					_log.warning(Poco::format("failed copy: %s", ex.displayText()));
				} catch (Poco::Exception ex) {
					_log.warning(Poco::format("failed copy: %s", ex.displayText()));
				}

				//_log.information(Poco::format("files: %s", path));
				File f(copyDir.path() + remotePath + ".part");
				File parent(Path(f.path()).parent());
				if (!parent.exists()) parent.createDirectories();
				try {
					Poco::URI uri(Poco::format("http://%s/download?path=%s", remote, remotePath));
					std::auto_ptr<std::istream> is(Poco::URIStreamOpener::defaultOpener().open(uri));
					Poco::FileOutputStream os(f.path());
					int size = Poco::StreamCopier::copyStream(*is.get(), os, 1024 * 1024);
					os.close();
					File rename(copyDir.path() + remotePath);
					if (rename.exists()) rename.remove();
					f.renameTo(rename.path());
					f.setLastModified(modified.timestamp());
					_log.information(Poco::format("remote file copy %s %d", remotePath, size));
					_copyRemoteFiles--;
				} catch (Poco::FileException& ex) {
					_log.warning(Poco::format("failed copy: %s", ex.displayText()));
				} catch (Poco::Exception ex) {
					_log.warning(Poco::format("failed copy: %s", ex.displayText()));
				}
			}
		}
	} catch (Poco::Exception ex) {
		_log.warning(Poco::format("failed files: %s", ex.displayText()));
	}
	return true;
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

	_copySize = 0;
	_copyProgress = 0;
	_currentCopyProgress = 0;
	_removableAlpha = 0.01f;
	_removableCover = 0;
	// Sleep(3000);
	// Sleep(60000);

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
		Sleep(1000);
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			for (vector<Container*>::iterator it = _contents.begin(); it != _contents.end(); it++) {
				(*it)->initialize();
			}
			_log.information("purge all contents");
			SAFE_RELEASE(_playlistName);
			SAFE_RELEASE(_currentName);
			SAFE_RELEASE(_nextPlaylistName);
			SAFE_RELEASE(_nextName);
			SAFE_RELEASE(_preparedPlaylistName);
			SAFE_RELEASE(_preparedName);
		}
		_status.clear();
		_playCurrent.action.clear();
		_playCurrent.transition.clear();
	} else {
		_running = false;
	}
	while (_removableAlpha > 0.0f) {
		Sleep(100);
	}
	_removableAlpha = 0;
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
					is.close();
					os.close();
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
			if (config().brightness > 0) config().brightness--;
			break;
		case 'X':
			if (config().brightness < 100) config().brightness++;
			break;
		case '0':
			if (_interrupttedContent) {
				_interrupttedContent->stop();
				_interrupttedContent = NULL;
			} else {
				map<string, ContainerPtr>::iterator it = _stanbyMedias.find("000");
				if (it != _stanbyMedias.end()) {
					_interrupttedContent = it->second;
					if (!_interrupttedContent->playing()) {
						_interrupttedContent->play();
					}
				}
			}
			break;
	}
	if (_brightness < config().brightness) {
		_brightness++;
	} else if (_brightness > config().brightness) {
		_brightness--;
	}

	// リムーバブルメディア検出
	if (_renderer.hasAddDrives()) {
		_removableIconAlpha += 0.01f;
		if (_removableIconAlpha >= 1.0f) _removableIconAlpha = 1;
	} else if (_addRemovable.empty()) {
		_removableIconAlpha -= 0.01f;
		if (_removableIconAlpha < 0.0f) _removableIconAlpha = 0.0f;
	}

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
			if (!_status["next-content"].empty() && _playCurrent.action.find("stop") == 0) {
				activePrepareNextContent(_playNext);
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

				if (_workspace->getScheduleCount() > 0) {
					Poco::LocalDateTime now;
					Poco::Timespan span(0, 0, 0, 10, 0);
					for (int i = 0; i < _workspace->getScheduleCount(); i++) {
						SchedulePtr schedule = _workspace->getSchedule(i);
						if (schedule->matchPast(now + span)) {
							string command = schedule->command();
							string t = Poco::DateTimeFormatter::format(now, "%Y/%m/%d(%w) %H:%M:%S");
							_log.information(Poco::format("%s %s", t, command));
							if (command.find("playlist ") == 0) {
								string playlistID = command.substr(9);
								playlist = _workspace->getPlaylist(playlistID);
							} else if (command.find("brightness ") == 0) {
								int brightness = -1;
								if (Poco::NumberParser::tryParse(command.substr(10), brightness) && brightness >= 0 && brightness <= 100) {
									setBrightness(brightness);
								}
							}
						}
					}
				}

				if (playlist) {
					PlayParameters args;
					args.playlistID = playlist->id();
					args.i = 0;
					_log.information(Poco::format("auto preparing: %s", playlist->id()));
					activePrepareNextContent(args);
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
		_status["action"] = _playCurrent.action;
		_status["transition"] = _playNext.transition;
		for (vector<Container*>::iterator it = _contents.begin(); it != _contents.end(); it++) {
			(*it)->process(_frame);
		}

		ContentPtr currentContent = NULL;
		if (_currentContent >= 0) currentContent = _contents[_currentContent]->get(0);

		if (currentContent) {
			if (_playCurrent.action == "stop" || _playCurrent.action == "stop-prepared") {
				// 停止系

			} else if (_playCurrent.action == "wait-prepared") {
				// 次のコンテンツが準備でき次第切替
				if (!_status["next-content"].empty()) {
					// _log.information("wait prepared next content, prepared now.");
					_doSwitchNext = true;
				}
			} else {
				// コマンド指定が無ければ現在再生中のコンテンツの終了を待つ
				int next = (_currentContent + 1) % _contents.size();
				ContainerPtr nextContainer = _contents[next];
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
				ContainerPtr oldNextContainer = NULL;
				{
					Poco::ScopedLock<Poco::FastMutex> lock(_lock);
					int next = (_currentContent + 1) % _contents.size();
					oldNextContainer = _contents[next];
					_contents[next] = _prepared;
					_prepared = NULL;
					LPDIRECT3DTEXTURE9 tmp = _nextPlaylistName;
					_nextPlaylistName = _preparedPlaylistName;
					_preparedPlaylistName = tmp;
					tmp = _nextName;
					_nextName = _preparedName;
					_preparedName = tmp;
				}
				if (oldNextContainer) _delayReleases.push_back(oldNextContainer);
				Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
				PlayListPtr playlist = _workspace->getPlaylist(_playPrepared.playlistID);
				if (playlist && playlist->itemCount() > _playPrepared.i) {
					PlayListItemPtr item = playlist->items()[_playPrepared.i];
					if (item) {
						_log.information(Poco::format("switch content: %s-%d: %s", _playPrepared.playlistID, _playPrepared.i, item->media()->name()));
						{
							Poco::ScopedLock<Poco::FastMutex> lock(_lock);
							_playNext = _playPrepared;
						}
					}
				} else {
					_log.warning(Poco::format("not find playlist: %s", _playPrepared.i));
				}
				_status["next-playlist"] = _status["prepared-playlist"];
				_status["next-content-id"] = _status["prepared-content-id"];
				_status["next-content"] = _status["prepared-content"];
				LPDIRECT3DTEXTURE9 oldPreparedPlaylistName = NULL;
				LPDIRECT3DTEXTURE9 oldPreparedName = NULL;
				TransitionPtr oldTransition = NULL;
				{
					Poco::ScopedLock<Poco::FastMutex> lock(_lock);
					oldPreparedPlaylistName = _preparedPlaylistName;
					_preparedPlaylistName = NULL;
					oldPreparedName = _preparedName;
					_preparedName = NULL;
					oldTransition = _transition;
					_transition = NULL;
				}
				SAFE_RELEASE(oldPreparedPlaylistName);
				SAFE_RELEASE(oldPreparedName);
				SAFE_DELETE(oldTransition);
				removeStatus("prepared-playlist");
				removeStatus("prepared-content");
				_doSwitchNext = true;

			} else {
				// 準備済みコンテンツが無い場合は次のコンテンツへ切替
				int next = (_currentContent + 1) % _contents.size();
				ContainerPtr tmp = _contents[next];
				if (tmp && tmp->size() > 0) {
					_log.information("switch next contents");
					TransitionPtr oldTransition = NULL;
					{
						Poco::ScopedLock<Poco::FastMutex> lock(_lock);
						oldTransition = _transition;
						_transition = NULL;
					}
					SAFE_DELETE(oldTransition);
					_doSwitchNext = true;
				}
			}
			_nextStack.clear();
			_doSwitchPrepared = false;
		}

		// bool prepareNext = false;
		if (_doSwitchNext && !_transition) {
			//_doSwitchNext = false;
			_playCurrent = _playNext;
			int next = (_currentContent + 1) % _contents.size();
			ContentPtr nextContent = _contents[next]->get(0);
			if (nextContent && !nextContent->opened().empty()) {
				_doSwitchNext = false;
				_currentContent = next;
				_contents[next]->play();
				LPDIRECT3DTEXTURE9 oldPlaylistName = NULL;
				LPDIRECT3DTEXTURE9 oldCurrentName = NULL;
				{
					Poco::ScopedLock<Poco::FastMutex> lock(_lock);
					oldPlaylistName = _playlistName;
					_playlistName = _nextPlaylistName;
					_nextPlaylistName = NULL;
					oldCurrentName = _currentName;
					_currentName = _nextName;
					_nextName = NULL;
				}
				SAFE_RELEASE(oldPlaylistName);
				SAFE_RELEASE(oldCurrentName);
				_status["current-playlist"] = _status["next-playlist"];
				_status["current-content-id"] = _status["next-content-id"];
				_status["current-content"] = _status["next-content"];
				removeStatus("next-playlist");
				removeStatus("next-content-id");
				removeStatus("next-content");
				_playCount++;

//				if (_transition) SAFE_DELETE(_transition);
				if (currentContent) {
					Poco::ScopedLock<Poco::FastMutex> lock(_lock);
					if (_playCurrent.transition == "slide") {
						int cw = config().splitSize.cx;
						int ch = config().splitSize.cy;
						_transition = new SlideTransition(currentContent, nextContent, 0, ch);
					} else if (_playCurrent.transition == "dissolve") {
						_transition = new DissolveTransition(currentContent, nextContent);
					}
					if (_transition) _transition->initialize(_frame);
				}
				if (!_transition) {
					_doPrepareNext = true;
				}
			} else {
			}
		}

		if (_transition) {
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			if (_transition && _transition->process(_frame)) {
				// トランジション終了
				SAFE_DELETE(_transition);
				// 次のコンテンツ準備
				_doPrepareNext = true;
			}
		}

		if (_doPrepareNext && !_preparingNext) {
			_doPrepareNext = false;
			PlayParameters params;
			{
				Poco::ScopedLock<Poco::FastMutex> lock(_lock);
				params.playlistID = _playCurrent.playlistID;
				params.i = _playCurrent.i + 1;
				params.action = _playCurrent.action;
			}
			_nextStack.push_back(params);
			_nextStackTime = 0;
		}

		{
			// 次コンテンツのスタック処理
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			if (!_nextStack.empty()) {
				_nextStackTime++;
				if (_nextStackTime > 30) {
					activePrepareNextContent(_nextStack.back());
					_nextStack.clear();
				}
			}
		}

		{
			// プレイリスト切替のスタック処理
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			if (!_prepareStack.empty()) {
				_prepareStackTime++;
				if (_prepareStackTime > 30) {
					activePrepareContent(_prepareStack.back());
					_prepareStack.clear();
				}
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
			if (schedule->match(now + span)) {
				// 10秒前チェック
				string command = schedule->command();
				if (command.find("playlist ") == 0) {
					string playlistID = command.substr(9);
					PlayListPtr playlist = _workspace->getPlaylist(playlistID);
					if (playlist) {
						_log.information(Poco::format("[%s]exec %s", _nowTime, command));
						stackPrepareContent(playlistID);
					} else {
						_log.warning(Poco::format("[%s]failed command: %s", _nowTime, command));
					}
					break;
				} else {
					_log.warning(Poco::format("[%s]failed command: %s", _nowTime, command));
				}
			} else if (schedule->match(now)) {
				// 実時間チェック
				string command = schedule->command();
				if (command.find("playlist ") == 0) {
					if (_prepared) {
						activeSwitchContent();
						_log.information(Poco::format("[%s]exec %s", _nowTime, command));
						//_doSwitchPrepared = true;
					} else {
						_log.warning(Poco::format("[%s]failed next content not prepared %s", _nowTime, command));
					}
					break;
				} else if (command.find("brightness ") == 0) {
					int brightness = -1;
					if (Poco::NumberParser::tryParse(command.substr(10), brightness) && brightness >= 0 && brightness <= 100) {
						setBrightness(brightness);
					}
				}
			}
		}
	}

	if (_interrupttedContent) _interrupttedContent->process(_frame);

	_status["brightness"] = Poco::NumberFormatter::format(config().brightness);
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
	if (_interrupttedContent) _interrupttedContent->draw(_frame);

	if (_brightness < 100) {
		DWORD col = ((DWORD)(0xff * (100 - _brightness) / 100) << 24) | 0x000000;
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
			_status["time_current"] = c->get("time_current");
			_status["time_remain"] = c->get("time_remain");
			status2 = Poco::format("%04d/%04d %s %s", current, duration, time, buffers);
		}
	}

	if (config().viewStatus) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		_renderer.drawFontTextureText(0, config().subRect.bottom - 128, 24, 32, 0x996699ff, config().name);
		_renderer.drawFontTextureText(0, config().subRect.bottom - 96, 12, 16, 0x99ffffff, status1);
		_renderer.drawFontTextureText(0, config().subRect.bottom - 80, 12, 16, 0x99ccccff, "frame");
		_renderer.drawFontTextureText(120, config().subRect.bottom - 80, 12, 16, 0x99ccccff, "time");
		_renderer.drawFontTextureText(264, config().subRect.bottom - 80, 12, 16, 0x99ccccff, "remain");
		_renderer.drawFontTextureText(0, config().subRect.bottom - 64, 12, 16, 0x99ffffff, status2);
		_renderer.drawFontTextureText(0, config().subRect.bottom - 48, 12, 16, 0x99ccccff, "action");
		if (!_playCurrent.action.empty()) _renderer.drawFontTextureText(84, config().subRect.bottom - 48, 12, 16, 0x99ffffff, _playCurrent.action);
		_renderer.drawFontTextureText(264, config().subRect.bottom - 48, 12, 16, 0x99ccccff, "transition");
		if (!_playNext.transition.empty()) _renderer.drawFontTextureText(396, config().subRect.bottom - 48, 12, 16, 0x99ffffff, _playNext.transition);

		_renderer.drawFontTextureText(600, config().subRect.bottom - 112, 12, 16, 0x99ccccff, " current");
		_renderer.drawTexture(700, config().subRect.bottom - 112, _playlistName, 0xccffffff, 0xccffffff,0x99ffffff, 0x99ffffff);
		_renderer.drawTexture(700, config().subRect.bottom - 96, _currentName, 0xccffffff, 0xccffffff,0x99ffffff, 0x99ffffff);
		_renderer.drawFontTextureText(600, config().subRect.bottom - 80, 12, 16, 0x99ccccff, "    next");
		_renderer.drawTexture(700, config().subRect.bottom - 80, _nextName, 0xccffffff, 0xccffffff,0x99ffffff, 0x99ffffff);
		_renderer.drawFontTextureText(600, config().subRect.bottom - 64, 12, 16, 0x99ccccff, "prepared");
		_renderer.drawTexture(700, config().subRect.bottom - 64, _preparedPlaylistName, 0xccffffff, 0xccffffff,0x99ffffff, 0x99ffffff);
		_renderer.drawTexture(700, config().subRect.bottom - 48, _preparedName, 0xccffffff, 0xccffffff,0x99ffffff, 0x99ffffff);

		int next = (_currentContent + 1) % _contents.size();
		string wait(_contents[next]->opened().empty()?"preparing":"ready");
		_renderer.drawFontTextureText(0, config().subRect.bottom - 32, 12, 16, 0x99ffffff, Poco::format("[%s] play contents:%04d copy<%d> playing<%d> next:%s", _nowTime, _playCount, _copyRemoteFiles, _currentContent, wait));
	}

	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		D3DSURFACE_DESC desc;
		HRESULT hr = _removableIcon->GetLevelDesc(0, &desc);
		int tw = desc.Width / 2;
		int th = desc.Height / 2;
		if (_removableIcon && _removableIconAlpha > 0) {
			DWORD col = ((DWORD)(0xff * _removableIconAlpha) << 24) | 0xffffff;
			_renderer.drawTexture(0, config().subRect.bottom - th, tw, th, _removableIcon, 0, col, col, col, col);
		}
		if (_removableAlpha > 0) {
			DWORD col = ((DWORD)(0x66 * _removableAlpha) << 24) | 0x333333;
			_renderer.drawTexture(tw + 1, config().subRect.bottom - th / 2, config().subRect.right - tw - 1, 10, NULL, 0, col, col, col, col);
			DWORD col1 = ((DWORD)(0x66 * _removableAlpha) << 24) | 0x33ccff;
			DWORD col2 = ((DWORD)(0x66 * _removableAlpha) << 24) | 0x3399cc;
			_renderer.drawTexture(tw + 2, config().subRect.bottom - th / 2 + 1, (config().subRect.right - tw - 2) * _currentCopyProgress / 100, 8, NULL, 0, col1, col1, col2, col2);
//			_renderer.drawFontTextureText(tw, config().subRect.bottom - th / 2, 12, 16, 0xccffffff, Poco::format("%d %d(%d%%)", _currentCopySize, _copySize, _currentCopyProgress));
		}
	}
}
