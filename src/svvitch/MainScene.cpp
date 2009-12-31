#include "MainScene.h"

#include <Psapi.h>
#include <Poco/format.h>
#include <Poco/Exception.h>
#include <Poco/string.h>
#include <Poco/UnicodeConverter.h>
#include <Poco/Util/XMLConfiguration.h>
#include <Poco/LocalDateTime.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/Timespan.h>

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
	_frame(0), _luminance(100), _preparing(false), _playCount(0), _transition(NULL), _interruptMedia(NULL),
	_playlistName(NULL), _currentName(NULL), _nextPlaylistName(NULL), _nextName(NULL),
	_prepared(NULL), _preparedPlaylistName(NULL), _preparedName(NULL)
{
	_luminance = config().luminance;
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

	SAFE_DELETE(_workspace);

	_log.information("save configuration");
	try {
		Poco::Util::XMLConfiguration* xml = new Poco::Util::XMLConfiguration("switch-config.xml");
		if (xml) {
			xml->setInt("stage.luminnace", _luminance);
			xml->save("switch-config.xml");
			xml->release();
		}
	} catch (Poco::Exception& ex) {
		_log.warning(Poco::format("failed save configuration file: %s", ex.displayText()));
	}
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
	_log.information("*initialized MainScene");
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
	// 初期化フェーズ
	int count = 15;
	while (_transition || count-- > 0) {
		// トランジション中は解放しないようにする。更に初期化まで0.5秒くらいウェイトする
		if (_contents.empty()) return false;
		Poco::Thread::sleep(30);
	}

	// 準備フェーズ
	int next = (_currentContent + 1) % _contents.size();
	if (!_currentCommand.empty()) {
		int jump = _currentCommand.find_first_of("jump");
		if (jump == 0) {
			string s = Poco::trim(_currentCommand.substr(4));
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
		} else if (_currentCommand == "stop") {
			_contents[next]->initialize();
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
					_nextCommand = item->next();
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
	return true;
}

bool MainScene::stackPrepare(string& playlistID, int i) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	PrepareArgs args;
	args.playlistID = playlistID;
	args.i = i;
	_prepareStack.push_back(args);
	if (_prepareStack.size() > 5) _prepareStack.erase(_prepareStack.begin());
	_prepareStackTime = 0;
	return true;
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
//						DSContentPtr movie = new DSContent(_renderer);
						if (movie->open(media)) {
							movie->setPosition(config().stageRect.left, config().stageRect.top);
							movie->setBounds(config().stageRect.right, config().stageRect.bottom);
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
	if (_prepared) {
		PlayListPtr playlist = _workspace->getPlaylist(_preparedPlaylistID);
		if (_prepared && playlist && playlist->itemCount() > _preparedItem) {
			PlayListItemPtr item = playlist->items()[_preparedItem];
			if (item) {
				int next = (_currentContent + 1) % _contents.size();
				ContainerPtr tmp = _contents[next];
				_contents[next] = _prepared;
				_prepared = tmp;
				_log.information(Poco::format("switch content: %s-%d: %s", _preparedPlaylistID, _preparedItem, item->media()->name()));
				{
					Poco::ScopedLock<Poco::FastMutex> lock(_lock);
					LPDIRECT3DTEXTURE9 tmp = _nextPlaylistName;
					_nextPlaylistName = _preparedPlaylistName;
					_preparedPlaylistName = tmp;
					tmp = _nextName;
					_nextName = _preparedName;
					_preparedName = tmp;
				}
				_playlistID = _preparedPlaylistID;
				_playlistItem = _preparedItem;
				_nextCommand = item->next();
				_nextTransition = item->transition();
				_status["next-playlist"] = _status["prepared-playlist"];
				_status["next-content"] = _status["prepared-content"];
				_doSwitch = true;
				{
					Poco::ScopedLock<Poco::FastMutex> lock(_lock);
					SAFE_RELEASE(_preparedPlaylistName);
					SAFE_RELEASE(_preparedName);
				}
				{
					Poco::ScopedLock<Poco::FastMutex> lock(_lock);
					SAFE_DELETE(_transition);
				}
				SAFE_DELETE(_prepared);
				_status.erase(_status.find("prepared-playlist"));
				_status.erase(_status.find("prepared-content"));
				return true;
			}
		} else {
			_log.warning(Poco::format("not find playlist: %s", _preparedPlaylistID));
		}

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
			_doSwitch = true;
			return true;
		}
	}
	return false;
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


void MainScene::process() {
	switch (_keycode) {
		case 'Z':
			if (_luminance > 0) _luminance--;
			break;
		case 'X':
			if (_luminance < 100) _luminance++;
			break;
	}

	if (!_startup && _frame > 100) {
		_startup = true;
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
				}
			} else {
				_log.warning("no playlist, no auto starting");
			}
		}
	}
	if (_startup) {
		for (vector<Container*>::iterator it = _contents.begin(); it != _contents.end(); it++) {
			(*it)->process(_frame);
		}

		ContentPtr currentContent = NULL;
		if (_currentContent >= 0) currentContent = _contents[_currentContent]->get(0);

		if (currentContent) {
			if (_currentCommand == "stop") {

			} else if (_currentCommand == "wait-prepared") {
				if (!_status["next-content"].empty()) {
					// _log.information("wait prepared next content, prepared now.");
					_doSwitch = true;
				}
			} else {
				// コマンド指定が無ければ現在再生中のコンテンツの終了を待つ
				if (_contents[_currentContent]->finished()) {
					// _log.information(Poco::format("content[%d] finished: ", _currentContent));
					_doSwitch = true;
				}
			}
		} else {
			if (_autoStart && !_status["next-content"].empty()) {
				_log.information("auto start content");
				_doSwitch = true;
				_autoStart = false;
			}
		}

		bool prepareNext = false;
		if (_doSwitch && !_transition) {
			_doSwitch = false;
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
				_currentCommand = _nextCommand;
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
			if (_prepareStackTime > 30) {
				activePrepare(_prepareStack.back());
				_prepareStack.clear();
			}
		}
	}

	// ワークスペースの更新チェック
	if (_updatedWorkspace) {
		Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
		SAFE_DELETE(_workspace);
		_workspace = _updatedWorkspace;
		_updatedWorkspace = NULL;
		if (_currentCommand != "stop") {
			_playlistItem--;
			activePrepareNextMedia();
		}
		if (_prepared) {
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
					_log.information(Poco::format("[%s]exec %s", _nowTime, command));
					stackPrepare(command.substr(9));
					break;
				}
			} else if (schedule->check(now)) {
				// 実時間チェック
				string command = schedule->command();
				if (command.find("playlist ") == 0) {
					if (_prepared) {
						_log.information(Poco::format("[%s]exec %s", _nowTime, command));
						switchContent();
					} else {
						_log.warning(Poco::format("[%s]failed next content not prepared %s", _nowTime, command));
					}
					break;
				}
			}
		}
	}

	_frame++;
}

void MainScene::draw1() {
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
//			MediaItemPtr media = _workspace.getMedia(c->opened());
			int current = c->current();
			int duration = c->duration();
			string time;
			FFMovieContentPtr movie = dynamic_cast<FFMovieContentPtr>(c);
			if (movie != NULL) {
				DWORD currentTime = movie->currentTime();
				DWORD timeLeft = movie->timeLeft();
				Uint32 fps = movie->getFPS();
				float avgTime = movie->getAvgTime();
				status1 = Poco::format("%03lufps(%03.2hfms)", fps, avgTime);
				time = Poco::format("%02lu:%02lu.%03lu %02lu:%02lu.%03lu", currentTime / 60000, currentTime / 1000 % 60, currentTime % 1000 , timeLeft / 60000, timeLeft / 1000 % 60, timeLeft % 1000);
			} else {
//				LPDIRECT3DTEXTURE9 name = _renderer.getCachedTexture(media->id());
//				_renderer.drawTexture(0, 580, name, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
			}
			status2 = Poco::format("%04d/%04d %s", current, duration, time);
		}
	}

	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		_renderer.drawFontTextureText(0, 640, 12, 16, 0xccffffff, status1);
		_renderer.drawFontTextureText(0, 660, 12, 16, 0xccffffff, status2);
		if (!_currentCommand.empty()) _renderer.drawFontTextureText(0, 680, 12, 16, 0xccffffff, Poco::format("next>%s", _currentCommand));
		if (!_nextTransition.empty()) _renderer.drawFontTextureText(0, 700, 12, 16, 0xccffffff, Poco::format("transition>%s", _nextTransition));

		_renderer.drawTexture(500, 640, _playlistName, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
		_renderer.drawTexture(500, 655, _currentName, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
		_renderer.drawTexture(500, 670, _nextName, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
		_renderer.drawTexture(500, 685, _preparedPlaylistName, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
		_renderer.drawTexture(500, 700, _preparedName, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);

		int next = (_currentContent + 1) % _contents.size();
		string wait(_contents[next]->opened().empty()?"preparing":"ready");
		_renderer.drawFontTextureText(0, 730, 12, 16, 0xccffffff, Poco::format("[%s] play contents:%04d playing no<%d> next:%s", _nowTime, _playCount, _currentContent, wait));
	}
}