#include "MainScene.h"

#include <algorithm>
#include <Psapi.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/format.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeParser.h>
#include <Poco/Exception.h>
#include <Poco/string.h>
#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>
#include <Poco/UnicodeConverter.h>
#include <Poco/Util/XMLConfiguration.h>
#include <Poco/LocalDateTime.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/Timespan.h>
#include <Poco/Timezone.h>
#include <Poco/FileStream.h>
#include <Poco/URIStreamOpener.h>
#include <Poco/URI.h>

#include "ImageContent.h"
#ifdef USE_FFMPEG
#include "FFMovieContent.h"
#endif
#include "TextContent.h"
#include "DSContent.h"
#include "SlideTransition.h"
#include "DissolveTransition.h"
#include "Schedule.h"

#include "Utils.h"

#include "CaptureContent.h"
#include "FlashContent.h"
#include "IEContent.h"
#include "MixContent.h"
#ifdef USE_OPENCV
#include "CvContent.h"
#endif

using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::NodeList;


MainScene::MainScene(Renderer& renderer): Scene(renderer),
	_workspace(NULL), _updatedWorkspace(NULL),
	activePrepareContent(this, &MainScene::prepareContent),
	activePrepareNextContent(this, &MainScene::prepareNextContent),
	activeSwitchContent(this, &MainScene::switchContent),
	activeCopyRemote(this, &MainScene::copyRemote),
	activeAddRemovableMedia(this, &MainScene::addRemovableMedia),
	_frame(0), _brightness(100), _preparing(false), _playCount(0),
	_doPrepareNext(false), _preparingNext(false), _doSwitchNext(false), _doSwitchPrepared(false), _transition(NULL),
	_description(NULL), _playlistName(NULL), _currentName(NULL), _nextPlaylistName(NULL), _nextName(NULL),
	_prepared(NULL), _preparedPlaylistName(NULL), _preparedName(NULL),
	_initializing(false), _running(false), _castLog(NULL),
	_removableIcon(NULL), _removableIconAlpha(0), _removableAlpha(0), _removableCover(0), _copySize(0), _currentCopySize(0), _copyProgress(0), _currentCopyProgress(0),
	_delayedCopy(false), _copyRemoteFiles(0), _interrupttedContent(NULL), _messageFrame(0)
{
	initialize();
}

MainScene::~MainScene() {
	_log.information("release contents");
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	for (vector<Container*>::iterator it = _contents.begin(); it != _contents.end(); it = _contents.erase(it)) {
		SAFE_DELETE(*it);
	}
	_doSwitchPrepared = false;
	if (_castLog) {
		_castLog->close();
		SAFE_DELETE(_castLog);
	}
	execDelayedRelease();
	SAFE_DELETE(_prepared);
	SAFE_DELETE(_transition);

	SAFE_RELEASE(_playlistName);
	SAFE_RELEASE(_currentName);
	SAFE_RELEASE(_nextPlaylistName);
	SAFE_RELEASE(_nextName);
	SAFE_RELEASE(_preparedName);
	SAFE_RELEASE(_removableIcon);

	while (!_deletes.empty()) {
		string path = _deletes.front();
		_deletes.pop();
		try {
			File f(path);
			if (f.exists()) {
				try {
					f.remove();
					_log.information(Poco::format("file delete(not used): %s", f.path()));
				} catch (Poco::FileException& ex) {
					_log.warning(Poco::format("failed delete: ", ex.displayText()));
				}
			}
		} catch (const std::exception& ex) {
		}
	}
	SAFE_DELETE(_workspace);
	preparedStanbyMedia();
	_log.information("*release main-scene");
}

void MainScene::execDelayedRelease() {
	Poco::DateTime now;
	Poco::Timestamp t = now.timestamp();
	int count = 0;
	vector<ContainerPtr> deletes;
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		for (vector<DelayedRelease>::iterator it = _delayReleases.begin(); it != _delayReleases.end();) {
			DelayedRelease dr = *it;
			if (t - dr.timestamp() > 2000000) {
				if (!_transition || !_transition->use(dr.container())) {
					deletes.push_back(dr.container());
					it = _delayReleases.erase(it);
					continue;
				}
			}
			++it;
		}
	}
	for (vector<ContainerPtr>::iterator it = deletes.begin(); it != deletes.end(); ++it) {
		SAFE_DELETE(*it);
		count++;
		Poco::Thread::sleep(0);
	}
	_log.information(Poco::format("delayed release: %d", count));
}

void MainScene::pushDelayedRelease(ContainerPtr c) {
	_delayReleases.push_back(DelayedRelease(c));
}

bool MainScene::initialize() {
#ifdef USE_FFMPEG
	avcodec_register_all();
	avdevice_register_all();
	av_register_all();
#endif

	_contents.clear();
	_contents.push_back(new Container(_renderer));
	_contents.push_back(new Container(_renderer)); // 2個のContainer
	_currentContent = -1;

	while (!_delayUpdateFiles.empty()) updateDelayedFiles();

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

	setDescription(config().description);

	LPDIRECT3DTEXTURE9 texture = _renderer.createTexture("images/Crystal_Clear_device_usbpendrive_unmount.png");
	if (texture) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		_removableIcon = texture;
	}

	if (config().outCastLog) {
		File logDir("logs/");
		if (!logDir.exists()) logDir.createDirectories();
	}

	WorkspacePtr workspace = new Workspace(config().workspaceFile);
	if (workspace->parse()) {
		_workspace = workspace;
		setStatus("workspace", _workspace->signature());
		preparedStanbyMedia();
		preparedFont(_workspace);
	} else {
		_log.warning("failed parse workspace");
	}
	setStatus("stage-name", config().name);
	setStatus("stage-description", config().description);
	_timeSecond = -1;
	_frame = 0;
	_brightness = config().brightness;
	_running = true;
	_pause = false;
	_startup = false;
	_autoStart = false;
	_log.information("*created main-scene");
	return true;
}

void MainScene::preparedFont(WorkspacePtr workspace) {
	if (workspace) {
		vector<string> fonts = workspace->getFonts();
		for (vector<string>::iterator it = fonts.begin(); it != fonts.end(); it++) {
			_renderer.addPrivateFontFile(*it);
		}
		_renderer.getPrivateFontFamilies(fonts);
		for (vector<string>::iterator it = fonts.begin(); it != fonts.end(); it++) {
			_log.information(Poco::format("font: %s", (*it)));
		}
	}
}

void MainScene::preparedStanbyMedia() {
	Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
	for (map<string, ContainerPtr>::iterator it = _stanbyMedias.begin(); it != _stanbyMedias.end(); it = _stanbyMedias.erase(it)) {
		SAFE_DELETE(it->second);
	}
	//if (_workspace) {
		//for (int i = 0; i < _workspace->getMediaCount(); i++) {
		//	MediaItemPtr media = _workspace->getMedia(i);
			//if (media->stanby()) {
			//	ContainerPtr c = new Container(_renderer);
			//	if (prepareMedia(c, media, "")) {
			//		_stanbyMedias[media->id()] = c;
			//		_log.information(Poco::format("standby media: %s", media->id()));
			//	}
			//}
		//}
	//}
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
	ContainerPtr tmp = new Container(_renderer);
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		int next = (_currentContent + 1) % _contents.size();
		ContainerPtr  oldNextContainer = _contents[next];
		if (oldNextContainer) pushDelayedRelease(oldNextContainer);
		_contents[next] = tmp;
	}

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

	ContainerPtr c = new Container(_renderer);
	if (preparePlaylist(c, playlistID, i, true)) {
		string playlistName = "ready";
		string itemID = "";
		string itemName = "ready";
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
			PlayListPtr playlist = _workspace?_workspace->getPlaylist(playlistID):NULL;
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
		LPDIRECT3DTEXTURE9 oldNextPlaylistName = NULL;
		LPDIRECT3DTEXTURE9 oldNextName = NULL;
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			int next = (_currentContent + 1) % _contents.size();
			ContainerPtr  oldNextContainer = _contents[next];
			if (oldNextContainer) pushDelayedRelease(oldNextContainer);
			_contents[next] = c;
			oldNextPlaylistName = _nextPlaylistName;
			_nextPlaylistName = t1;
			oldNextName = _nextName;
			_nextName = t2;
		}
		SAFE_RELEASE(oldNextPlaylistName);
		SAFE_RELEASE(oldNextName);
		_status["next-playlist-id"] = playlistID;
		_status["next-playlist"] = playlistName;
		_status["next-content-id"] = itemID;
		_status["next-content"] = itemName;
	} else {
		SAFE_DELETE(c);
		_log.warning(Poco::format("failed prepare: %s-%d", playlistID, i));
	}
	_preparingNext = false;
	execDelayedRelease();
	return true;
}

bool MainScene::stackPrepareContent(string& playlistID, int i) {
	PlayParameters args;
	args.playlistID = playlistID;
	args.i = i;
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		_prepareStack.push_back(args);
		if (_prepareStack.size() > 5) _prepareStack.erase(_prepareStack.begin());
	}
	_prepareStackTime = 0;
	return true;
}

const void MainScene::setDescription(const string& description) {
	if (!_description || config().description != description) {
		config().description = description;
		LPDIRECT3DTEXTURE9 t = _renderer.createTexturedText(L"", 30, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, description);
		LPDIRECT3DTEXTURE9 old = _description;
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			_description = t;
		}
		SAFE_RELEASE(old);
	}
}

const string MainScene::getPlaylistText(const string& playlistID) {
	Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
	PlayListPtr playlist = _workspace?_workspace->getPlaylist(playlistID):NULL;
	if (playlist) {
		return playlist->text();
	}
	return "";
}

bool MainScene::setPlaylistText(const string& playlistID, const string& text) {
	Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
	PlayListPtr playlist = _workspace?_workspace->getPlaylist(playlistID):NULL;
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
	removeStatus("prepared-playlist-id");
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
	if (preparePlaylist(c, params.playlistID, params.i, false)) {
		string playlistID = params.playlistID;
		string playlistName = "ready";
		string itemID = "";
		string itemName = "ready";
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
			PlayListPtr playlist = _workspace?_workspace->getPlaylist(playlistID):NULL;
			if (playlist && playlist->itemCount() > 0 && playlist->itemCount() > params.i) {
				playlistName = playlist->name();
				PlayListItemPtr item = playlist->items()[params.i];
				itemID = item->media()->id();
				itemName = item->media()->name();
				_playPrepared.playlistID = playlistID;
				_playPrepared.i = params.i;
				_playPrepared.action = item->next();
				_playPrepared.transition = item->transition();
			} else {
				_log.warning(Poco::format("failed playlist item index: %s-%d", params.playlistID, params.i));
				SAFE_DELETE(c);
				return false;
			}
		}

		LPDIRECT3DTEXTURE9 t1 = _renderer.createTexturedText(L"", 14, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, playlistName);
		LPDIRECT3DTEXTURE9 t2 = _renderer.createTexturedText(L"", 14, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, itemName);
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			_prepared = c;
			_preparedPlaylistName = t1;
			_preparedName = t2;
		}
		_status["prepared-playlist-id"] = playlistID;
		_status["prepared-playlist"] = playlistName;
		_status["prepared-content-id"] = itemID;
		_status["prepared-content"] = itemName;
		return true;
	} else {
		_log.warning(Poco::format("failed prepareContent: %s-%d", params.playlistID, params.i));
	}
	SAFE_DELETE(c);
	return false;
}

bool MainScene::preparePlaylist(ContainerPtr container, const string& playlistID, const int i, const bool round) {
	_log.information(Poco::format("prepare: %s-%d", playlistID, i));
	PlayListPtr playlist = NULL;
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
		playlist = _workspace?_workspace->getPlaylist(playlistID):NULL;
	}
	container->initialize();
	if (playlist && playlist->itemCount() > 0 && (round || playlist->itemCount() > i)) {
		int index = i;
		if (round) index = i % playlist->itemCount();
		PlayListItemPtr item = playlist->items().at(index);
		MediaItemPtr media = item->media();
		if (media) {
			return prepareMedia(container, media, playlist->text());
		} else {
			_log.warning("failed prepare next media, no media item");
		}
	} else {
		_log.warning(Poco::format("failed preparing item index: %s-%d", playlistID, i));
	}
	return false;
}

bool MainScene::prepareMedia(ContainerPtr container, MediaItemPtr media, const string& templatedText) {
	//_log.information(Poco::format("file: %d", media->fileCount()));
	float x = F(config().stageRect.left);
	float y = F(config().stageRect.top);
	float w = F(config().stageRect.right);
	float h = F(config().stageRect.bottom);
	switch (media->type()) {
		case MediaTypeMix:
			{
				MixContentPtr mix = new MixContent(_renderer, config().splitType);
				if (mix->open(media)) {
					mix->setPosition(x, y);
					mix->setBounds(w, h);
					container->add(mix);
				} else {
					SAFE_DELETE(mix);
				}
			}
			break;

		case MediaTypeImage:
			{
				ImageContentPtr image = new ImageContent(_renderer, config().splitType);
				if (image->open(media)) {
					image->setPosition(x, y);
					image->setBounds(w, h);
					container->add(image);
				} else {
					SAFE_DELETE(image);
				}
			}
			break;

		case MediaTypeMovie:
			{
				for (vector<string>::iterator it = config().movieEngines.begin(); it < config().movieEngines.end(); it++) {
					string engine = Poco::toLower(*it);
#ifdef USE_FFMPEG
					if (engine == "ffmpeg") {
						FFMovieContentPtr movie = new FFMovieContent(_renderer, config().splitType);
						if (movie->open(media)) {
							movie->setPosition(x, y);
							movie->setBounds(w, h);
							// movie->set("aspect-mode", "fit");
							container->add(movie);
							break;
						} else {
							SAFE_DELETE(movie);
						}
					} else if (engine == "directshow") {
#else
					if (engine == "directshow") {
#endif
						DSContentPtr movie = new DSContent(_renderer, config().splitType);
						if (movie->open(media)) {
							movie->setPosition(x, y);
							movie->setBounds(w, h);
							// movie->set("aspect-mode", "fit");
							container->add(movie);
							break;
						} else {
							SAFE_DELETE(movie);
						}
					} else {
						_log.warning(Poco::format("failed not found movie engine: %s", engine));
					}
				}
			}
			break;

		case MediaTypeText:
			break;

		case MediaTypeFlash:
			{
				FlashContentPtr flash = new FlashContent(_renderer, config().splitType, x, y, w, h);
				if (flash->open(media)) {
					container->add(flash);
				} else {
					SAFE_DELETE(flash);
				}
			}
			break;

		case MediaTypeBrowser:
			{
				IEContentPtr ie = new IEContent(_renderer, config().splitType, x, y, w, h);
				if (ie->open(media)) {
					container->add(ie);
				} else {
					SAFE_DELETE(ie);
				}
			}
			break;

		case MediaTypeCvCap:
			{
				CaptureContentPtr cvcap = new CaptureContent(_renderer, config().splitType);
				if (cvcap->open(media)) {
					container->add(cvcap);
				} else {
					SAFE_DELETE(cvcap);
				}
			}
			break;

#ifdef USE_OPENCV
		case MediaTypeCv:
			{
				CvContentPtr cv = new CvContent(_renderer, config().splitType);
				if (cv->open(media)) {
					container->add(cv);
				} else {
					SAFE_DELETE(cv);
				}
			}
			break;
#endif
		default:
			_log.warning("media type: unknown");
	}
	if (media->containsFileType(MediaTypeText)) {
		_log.information("contains text");
		vector<TextContentPtr> ref;
		int j = 0;
		int block = -1;
		for (int i = 0; i < media->fileCount(); i++) {
			MediaItemFile mif = media->files().at(i);
			if (mif.type() == MediaTypeText) {
				TextContentPtr text = new TextContent(_renderer, config().splitType);
				if (text->open(media, i)) {
					if (block <= 0 || ref.empty()) {
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
		drawConsole(Poco::format("failed prepare next media: %s", media->id()));
	}
	return false;
}

bool MainScene::switchContent() {
	_doSwitchPrepared = true;
	while (_doSwitchPrepared) Poco::Thread::sleep(20);
	return true;
}

bool MainScene::addStock(const string& path, File src, bool copy) {
	try {
		File parent(Path(Path(config().stockRoot, path).toString()).makeParent());
		//_log.information(Poco::format("parent: %s", parent.path()));
		if (!parent.exists()) {
			parent.createDirectories();
			_log.information(Poco::format("create directory: %s", parent.path()));
		}
		File dst(Path(config().stockRoot, Path(path).toString()).toString());
		_log.information(Poco::format("add stock[%s] -> %s", path, dst.path()));
		if (dst.exists()) {
			dst.remove();
			_log.information(Poco::format("deleted already stock file: %s", dst.path()));
		}
		if (copy) {
			// copy mode
			_log.information("not supported copy mode");
			return false;

		} else {
			// move mode
			src.renameTo(dst.path());
		}
		_stock[path] = dst;
		return true;
	} catch (Poco::FileException& ex) {
		_log.warning(Poco::format("failed add stock file[%s]: %s", path, ex.displayText()));
	} catch (Poco::PathSyntaxException& ex) {
		_log.warning(Poco::format("failed add stock file[%s]: %s", path, ex.displayText()));
	}
	return false;
}

void MainScene::clearStock() {
	for (map<string, File>::const_iterator it = _stock.begin(); it != _stock.end();) {
		File f = it->second;
		if (f.exists()) {
			try {
				f.remove();
			} catch (Poco::FileException& ex) {
				_log.warning(Poco::format("failed not remove stock file: %s", it->first));
			}
		}
		it = _stock.erase(it);
	}
	_log.information("clear stock");
}

bool MainScene::flushStock() {
	Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
	_log.information("flush stock");
	for (map<string, File>::const_iterator it = _stock.begin(); it != _stock.end();) {
		File dst(Path(config().dataRoot, it->first).toString());
		_log.information(Poco::format("flush stock: %s", dst.path()));
		File parent(Path(dst.path()).makeParent());
		if (!parent.exists()) parent.createDirectories();
		File f = it->second;
		if (f.exists()) {
			if (!_deletes.empty()) {
				queue<string> deletes;
				while (!_deletes.empty()) {
					string path = _deletes.front();
					_deletes.pop();
					if (dst.path() != path) {
						deletes.push(path);
					} else {
						_log.information(Poco::format("clear delete: %s", path));							
						if (!_messages.empty()) _messages.pop();
					}
				}
				_deletes = deletes;
			}
			try {
				if (dst.exists()) dst.remove();
				f.renameTo(dst.path());
			} catch (Poco::FileException& ex) {
				_log.warning(Poco::format("failed not move stock file: %s <- %s", it->first, dst.path()));
				File tempFile(dst.path() + ".part");
				if (tempFile.exists()) {
					removeDelayedUpdateFile(tempFile);
					tempFile.remove();
				}
				try {
					f.renameTo(tempFile.path());
					addDelayedUpdateFile(tempFile);
				} catch (Poco::FileException& ex1) {
					//result = false;
				}
			}
		} else {
			_log.warning(Poco::format("file not found: %s", f.path()));
		}
		it = _stock.erase(it);
	}
	return true;
}

void MainScene::addDelayedUpdateFile(File& file) {
	Poco::ScopedLock<Poco::FastMutex> lock(_delayedUpdateLock);	
	_delayUpdateFiles.push_back(file);
	Path path(file.path());
	setStatus("delayed-update", path.getFileName());
	_log.information(Poco::format("add delayed update: %s", file.path()));
}

void MainScene::removeDelayedUpdateFile(File& file) {
	Poco::ScopedLock<Poco::FastMutex> lock(_delayedUpdateLock);	
	vector<File>::iterator it = std::find(_delayUpdateFiles.begin(), _delayUpdateFiles.end(), file);
	if (it != _delayUpdateFiles.end()) {
		_delayUpdateFiles.erase(it);
		_log.information(Poco::format("remove delayed update: %s", file.path()));
	}
}

void MainScene::updateDelayedFiles() {
	if (_delayUpdateFiles.empty()) {
		removeStatus("delayed-update");
	} else {
		Poco::ScopedLock<Poco::FastMutex> lock(_delayedUpdateLock);
		for (vector<File>::iterator it = _delayUpdateFiles.begin(); it != _delayUpdateFiles.end();) {
			string src = (*it).path();
			int pos = src.size() - 5;
			if ((*it).exists() && pos >= 0 && src.substr(pos) == ".part") {
				File dst(src.substr(0, pos));
				Path path(dst.path());
				try {
					if (dst.exists()) {
						dst.remove();
						_log.warning(Poco::format("delayed updating file none: %s", src));
					}
					(*it).renameTo(dst.path());
					_log.information(Poco::format("delayed file updated: %s", path.getFileName()));
					drawConsole(Poco::format("delayed file updated: %s", path.getFileName()));
					it = _delayUpdateFiles.erase(it);
					continue;
				} catch (Poco::FileException& ex) {
					_log.warning(Poco::format("failed delayed file updating[%s]: %s", src, ex.displayText()));
					setStatus("delayed-update", path.getFileName());
				}
			} else {
				_log.warning(Poco::format("not found delayed file: %s", src));
				it = _delayUpdateFiles.erase(it);
				continue;
			}
			it++;
		}
	}
}

bool MainScene::updateWorkspace() {
	_log.information("update workspace");
	if (_workspace && _workspace->checkUpdate()) {
		WorkspacePtr workspace = new Workspace(_workspace->file());
		if (workspace->parse()) {
			_log.information("updated workspace. repreparing next contents");
			_updatedWorkspace = workspace;
			preparedFont(workspace);
			removeStatus("workspace");
			return true;
		} else {
			_log.warning("failed update workspace.");
			SAFE_DELETE(workspace);
		}
	} else if (!_workspace) {
		WorkspacePtr workspace = new Workspace(config().workspaceFile);
		if (workspace->parse()) {
			_running = false;
			_workspace = workspace;
			setStatus("workspace", _workspace->signature());
			preparedStanbyMedia();
			preparedFont(_workspace);
			drawConsole("updated workspace");
			_frame = 0;
			_startup = false;
			_autoStart = false;
			_running = true;
			return true;
		} else {
			_log.warning("failed parse workspace");
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
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_delayedUpdateLock);
		if (!_copyingRemote.empty()) {
			_log.warning(Poco::format("already remote copying: %s", _copyingRemote));
			_copyingRemote = remote;
			_delayedCopy = true;
			return;
		}
		_copyingRemote = remote;
	}
	_log.information(Poco::format("remote copy: %s", remote));
	drawConsole("start remote copy");
	setRemoteStatus(remote, "delayed-update", "");
	setRemoteStatus(remote, "remote-copy", "1");
	File copyDir("copys");
	if (copyDir.exists()) {
		try {
			copyDir.remove(true);
		} catch (Poco::IOException& ex) {
			_log.warning(Poco::format("failed remove copy dir: %s", ex.displayText()));
		}
	}
	//bool result = copyRemoteDir(remote, "/");

	Path remoteWorkspace("tmp/workspace.xml");
	if (copyRemoteFile(remote, "/workspace.xml", remoteWorkspace)) {
		setRemoteStatus(remote, "remote-copy", "2");
		drawConsole("check remote workspace");
		vector<string> remoteFiles;
		try {
			Poco::XML::DOMParser parser;
			Document* doc = parser.parse(remoteWorkspace.toString());
			if (doc) {
				Element* mediaList = doc->documentElement()->getChildElement("medialist");
				if (mediaList) {
					NodeList* items = mediaList->getElementsByTagName("item");
					for (int i = 0; i < items->length(); i++) {
						Element* e = (Element*)items->item(i);

						NodeList* files = e->getElementsByTagName("*");
						for (int j = 0; j < files->length(); j++) {
							Element* e1 = (Element*)files->item(j);
							string file = e1->innerText();
							string params;
							if (file.find("?") != string::npos) {
								params = file.substr(file.find("?") + 1);
								file = file.substr(0, file.find("?"));
							}
							if (file.find("switch-data:/") == 0) {
								file = file.substr(13);
								vector<string>::iterator it = std::find(remoteFiles.begin(), remoteFiles.end(), file);
								if (it == remoteFiles.end()) remoteFiles.push_back(file);
							}
						}
						files->release();
					}
					items->release();
				}
				Element* fonts = doc->documentElement()->getChildElement("fonts");
				if (fonts) {
					NodeList* file = fonts->getElementsByTagName("file");
					for (int i = 0; i < file->length(); i++) {
						Element* e = (Element*)file->item(i);
						string file = e->innerText();
						if (file.find("switch-data:/") == 0) {
							file = file.substr(13);
						}
						vector<string>::iterator it = std::find(remoteFiles.begin(), remoteFiles.end(), file);
						if (it == remoteFiles.end()) remoteFiles.push_back(file);
					}
					file->release();
				}
				doc->release();

				setRemoteStatus(remote, "remote-copy", "3");
				if (!remoteFiles.empty()) {
					int size = remoteFiles.size();
					drawConsole(Poco::format("remote copy %d files...", size));
					_copyRemoteFiles = size;
					for (vector<string>::iterator it = remoteFiles.begin(); it != remoteFiles.end(); it++) {
						Path out(config().dataRoot, Path(*it).toString());
						_log.information(Poco::format("remote: %s", out.toString()));
						setRemoteStatus(remote, "remote-copy", "3:" + out.getFileName());
						if (copyRemoteFile(remote, *it, out, true)) {
							drawConsole(Poco::format("remote copy: %d%%", 100 * (size - _copyRemoteFiles) / size));
						}
						_copyRemoteFiles--;
					}
				}
				drawConsole("remote copy: 100%");
				_copyRemoteFiles = 0;
				setRemoteStatus(remote, "remote-copy", "4");
				File dst(config().workspaceFile);
				if (dst.exists()) dst.remove();
				File src(remoteWorkspace);
				src.renameTo(dst.path());
				if (updateWorkspace()) {
					setRemoteStatus(remote, "remote-copy", "5");
				}
			} else {
				_log.warning(Poco::format("failed parse: %s", remoteWorkspace.toString()));
			}
		} catch (Poco::Exception& ex) {
			_log.warning(ex.displayText());
		}
	}
	File tmp(remoteWorkspace);
	if (tmp.exists()) tmp.remove();
	setRemoteStatus(remote, "remote-copy", "10");
	drawConsole("remote copy finished");

	// 多重でremoteCopyが呼ばれていた場合は再実行
	bool delayedCopy = false;
	string copyingRemote;
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_delayedUpdateLock);
		if (_delayedCopy) {
			delayedCopy = true;
			_delayedCopy = false;
		}
		copyingRemote = _copyingRemote;
		_copyingRemote.clear();
	}
	if (delayedCopy) {
		drawConsole("retry remote copy");
		copyRemote(copyingRemote);
	}
}

bool MainScene::copyRemoteFile(const string& remote, const string& path, Path& out, bool equalityCheck) {
	Poco::DateTime modified;
	Poco::File::FileSize size = 0;
	try {
		Poco::URI uri(Poco::format("%s/files?path=%s", remote, path));
		std::auto_ptr<std::istream> is(Poco::URIStreamOpener::defaultOpener().open(uri));
		string result;
		Poco::StreamCopier::copyToString(*is.get(), result);
		//_log.debug(Poco::format("result: %s", result));
		map<string, string> m;
		svvitch::parseJSON(result, m);
		svvitch::parseJSON(m["files"], m);
		//for (map<string, string>::iterator it = m.begin(); it != m.end(); it++) {
		//	_log.information(Poco::format("[%s]=%s", it->first, it->second));
		//}
		int tz = 0;
		Poco::DateTimeParser::parse(Poco::DateTimeFormat::SORTABLE_FORMAT, m["modified"], modified, tz);
		int tzd = Poco::Timezone::tzd();
		modified.makeUTC(tzd);
		Poco::UInt64 num = 0;
		Poco::NumberParser::tryParseUnsigned64(m["size"], num);
		size = num;
		//_log.information(Poco::format("modified: %s", Poco::DateTimeFormatter::format(modified, Poco::DateTimeFormat::SORTABLE_FORMAT)));
		File outFile(out);
		if (equalityCheck && outFile.exists()) {
			long modifiedDiff = abs((long)((modified.timestamp() - outFile.getLastModified())));
			if (size == outFile.getSize() && modifiedDiff <= 1000) {
				// サイズと更新日時(秒精度)があっていれば同一ファイルとする
				_log.information(Poco::format("remote file already exists: %s", path));
				return false;
			}
		}
	} catch (Poco::SyntaxException& ex) {
		_log.warning(Poco::format("failed remote files(URI miss): %s", ex.displayText()));
	} catch (Poco::IOException& ex) {
		_log.warning(Poco::format("failed remote I/O: %s", ex.displayText()));
	} catch (Poco::Exception& ex) {
		_log.warning(Poco::format("failed remote files: %s", ex.displayText()));
	}

	File parent(Path(out.toString()).makeDirectory());
	if (!parent.exists()) parent.createDirectories();
	bool updating = false;
	File tempFile(out.toString() + ".part");
	if (tempFile.exists()) {
		try {
			tempFile.remove();
		} catch (Poco::IOException& ex) {
			_log.warning(Poco::format("failed remove temp dir: %s", ex.displayText()));
		}
	}
	try {
		Poco::URI uri(Poco::format("%s/download?path=%s", remote, path));
		std::auto_ptr<std::istream> is(Poco::URIStreamOpener::defaultOpener().open(uri));
		Poco::FileOutputStream os(tempFile.path());
		long readSize = Poco::StreamCopier::copyStream(*is.get(), os, 512 * 1024);
		os.close();
		tempFile.setLastModified(modified.timestamp());
		updating = true;
		File outFile(out);
		if (outFile.exists()) outFile.remove();
		tempFile.renameTo(out.toString());
		_log.information(Poco::format("remote file copy %s %Lu %ld", path, size, readSize));
		//drawConsole(Poco::format("remote copy: %s", out.getFileName()));
		return true;
	} catch (Poco::FileException& ex) {
		_log.warning(Poco::format("failed file: %s", ex.displayText()));
		if (updating && tempFile.exists()) {
			addDelayedUpdateFile(tempFile);
			setRemoteStatus(remote, "delayed-update", out.getFileName());
			drawConsole(Poco::format("delayed: %s", out.getFileName()));
			return true;
		}
	} catch (Poco::Exception& ex) {
		_log.warning(Poco::format("failed remote copy: %s", ex.displayText()));
	}
	return false;
}

void MainScene::setRemoteStatus(const string& remote, const string& name, const string& value) {
	try {
		Poco::URI uri(Poco::format("%s/set/status?n=%s&v=%s", remote, name, value));
		std::auto_ptr<std::istream> is(Poco::URIStreamOpener::defaultOpener().open(uri));
		//return true;
	} catch (Poco::Exception& ex) {
		_log.warning(Poco::format("failed remote status: %s", ex.displayText()));
	}
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
				} catch (Poco::FileException& ex) {
					_log.warning(ex.displayText());
				}
			}
		}
		++it;
	}
	return size;
}



void MainScene::setPause(bool sw) {
	_pause = sw;
}

void MainScene::process() {
	Poco::LocalDateTime now;
	switch (_keycode) {
	case 'Z':
		if (config().brightness > 0) config().brightness--;
		break;
	case 'X':
		if (config().brightness < 100) config().brightness++;
		break;
	case ' ':
		_pause = !_pause;
	}
	if (_brightness < config().brightness) {
		_brightness++;
	} else if (_brightness > config().brightness) {
		_brightness--;
	}

	// 割り込みコンテンツ
	string interruptted = getStatus("interruptted");
	if (interruptted != _interruptted) {
		_log.information(Poco::format("interruptted: %s", interruptted));
		if (interruptted.empty()) {
			if (_interrupttedContent) {
				_interrupttedContent->stop();
				_interrupttedContent = NULL;
			}
		} else {
			map<string, ContainerPtr>::iterator it = _stanbyMedias.find(interruptted);
			if (it != _stanbyMedias.end()) {
				_interrupttedContent = it->second;
				if (!_interrupttedContent->playing()) {
					// 終了していたら再生しなおす
					_interrupttedContent->play();
				}
			}
		}
		_interruptted = interruptted;
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
			if (_workspace) {
				vector<string> olds = _workspace->existsFiles();
				vector<string> newFiles = _updatedWorkspace->existsFiles();
				for (vector<string>::const_iterator it = olds.begin(); it != olds.end(); it++) {
					vector<string>::const_iterator i = std::find(newFiles.begin(), newFiles.end(), *it);
					if (i == newFiles.end()) {
						_deletes.push(*it);
						_log.information(Poco::format("file not used: %s", (*it)));
					}
				}
			}
			SAFE_DELETE(_workspace);
			_workspace = _updatedWorkspace;
			_updatedWorkspace = NULL;
			if (!_status["next-content"].empty() && _playCurrent.action.find("stop") == 0) {
				activePrepareNextContent(_playNext);
			}
			setStatus("workspace", _workspace->signature());
			drawConsole(Poco::DateTimeFormatter::format(now, "[%H:%M:%S]") + "workspace updated");
		}
	}
	// 旧ワークスペースの未使用ファイルを削除
	if (_frame % 50 == 0 && !_deletes.empty()) {
		Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
		string path = _deletes.front();
		_deletes.pop();
		if (path.find("http") != 0) {
			try {
				File f(path);
				if (f.exists()) {
					try {
						f.remove();
						_log.information(Poco::format("file delete(not used): %s", f.path()));
					} catch (Poco::FileException& ex) {
						_log.warning(Poco::format("failed delete(not used): ", ex.displayText()));
						_deletes.push(path);
						if (_messages.size() <= 1) drawConsole(Poco::format("failed delete: %s", path));
					}
				}
			} catch (...) {
				_log.warning(Poco::format("failed not delete(not used): %s", path));
			}
		}
	}

	if (!_running) return;

	if (!_startup && _frame > 100) {
		Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
		if (_workspace && _workspace->getPlaylistCount() > 0) {
			// playlistがある場合は最初のplaylistを自動スタートする
			PlayListPtr playlist = _workspace->getPlaylist(0);
			int item = 0;

			if (_workspace && _workspace->getScheduleCount() > 0) {
				Poco::LocalDateTime now;
				Poco::Timespan span(0, 0, 0, 10, 0);
				for (int i = 0; i < _workspace->getScheduleCount(); i++) {
					SchedulePtr schedule = _workspace->getSchedule(i);
					if (schedule->matchPast(now + span)) {
						string command = schedule->command();
						string t = Poco::DateTimeFormatter::format(now, "%Y/%m/%d(%w) %H:%M:%S");
						//_log.information(Poco::format("%s %s", t, command));
						if (command.find("playlist ") == 0) {
							string params = command.substr(9);
							string playlistID = params;
							item = 0;
							if (params.find("-") != string::npos) {
								playlistID = params.substr(0, params.find("-"));
								if (!Poco::NumberParser::tryParse(params.substr(params.find("-") + 1), item)) {
									// failed parse item no
								}
							}
							playlist = _workspace->getPlaylist(playlistID);
						} else if (command.find("next") == 0) {
							item++;

						} else if (command.find("brightness ") == 0) {
							int brightness = -1;
							if (Poco::NumberParser::tryParse(command.substr(10), brightness) && brightness >= 0 && brightness <= 100) {
								setBrightness(brightness);
								drawConsole(Poco::DateTimeFormatter::format(now, "[%H:%M:%S]") + Poco::format("set brightness %d", brightness));
							}
						}
					}
				}
			}

			if (playlist) {
				PlayParameters args;
				args.playlistID = playlist->id();
				args.i = item;
				_log.information(Poco::format("auto preparing: %s-%d", playlist->id(), item));
				drawConsole(Poco::DateTimeFormatter::format(now, "[%H:%M:%S]") + Poco::format("preparing playlist %s-%d", playlist->id(), item));
				activePrepareNextContent(args);
				_autoStart = true;
				_frame = 0;
				_startup = true;
			}
		} else {
			if (_frame == 101) _log.warning("no playlist, no auto starting");
			// _frame = 0;
		}

	} else if (_startup) {
		_status["action"] = _playCurrent.action;
		_status["transition"] = _playNext.transition;

		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			ContentPtr currentContent = NULL;
			if (_currentContent >= 0) currentContent = _contents[_currentContent]->get(0);
			if (!_pause)
			{
				for (vector<Container*>::iterator it = _contents.begin(); it != _contents.end(); it++) {
					(*it)->process(_frame);
				}
			}

			bool readyNext = !_status["next-content"].empty();
			if (currentContent) {
				if (_playCurrent.action == "stop" || _playCurrent.action == "stop-prepared") {
					// 停止系

				} else if (_playCurrent.action == "wait-prepared") {
					// 次のコンテンツが準備でき次第切替
					if (readyNext && !_transition) {
						// _log.information("wait prepared next content, prepared now.");
						_doSwitchNext = true;
					}
				} else {
					// コマンド指定が無ければ現在再生中のコンテンツの終了を待つ.準備できていれば切替
					if (_contents[_currentContent]->finished() && readyNext && !_transition) {
						// _log.information(Poco::format("content[%d] finished: ", _currentContent));
						_doSwitchNext = true;
					}
				}
			} else if (_autoStart && readyNext && _frame > 200) {
				_log.information("auto start content");
				_doSwitchNext = true;
				_autoStart = false;
			}
		}

		if (_doSwitchPrepared && !_preparingNext) {
			if (_prepared) {
				{
					Poco::ScopedLock<Poco::FastMutex> lock(_lock);
					int next = (_currentContent + 1) % _contents.size();
					ContainerPtr oldNextContainer = _contents[next];
					if (oldNextContainer) pushDelayedRelease(oldNextContainer);
					_contents[next] = _prepared;
					_prepared = NULL;
					LPDIRECT3DTEXTURE9 tmp = _nextPlaylistName;
					_nextPlaylistName = _preparedPlaylistName;
					_preparedPlaylistName = tmp;
					tmp = _nextName;
					_nextName = _preparedName;
					_preparedName = tmp;
				}
				Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
				PlayListPtr playlist = _workspace?_workspace->getPlaylist(_playPrepared.playlistID):NULL;
				if (playlist && playlist->itemCount() > 0 && playlist->itemCount() > _playPrepared.i) {
					PlayListItemPtr item = playlist->items()[_playPrepared.i];
					if (item) {
						_log.information(Poco::format("switch content: %s-%d: %s", _playPrepared.playlistID, _playPrepared.i, item->media()->name()));
						{
							Poco::ScopedLock<Poco::FastMutex> lock(_lock);
							_playNext = _playPrepared;
						}
					}
				} else {
					_log.warning(Poco::format("not find playlist: %s-%d", _playPrepared.playlistID, _playPrepared.i));
				}
				_status["next-playlist-id"] = _status["prepared-playlist-id"];
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
				removeStatus("prepared-playlist-id");
				removeStatus("prepared-playlist");
				removeStatus("prepared-content");
				_doSwitchNext = true;

			} else {
				// 準備済みコンテンツが無い場合は次のコンテンツへ切替
				int next = (_currentContent + 1) % _contents.size();
				ContainerPtr tmp = _contents[next];
				if (tmp && tmp->size() > 0) {
					_log.information("switch next contents");
					SAFE_DELETE(_transition);
					_doSwitchNext = true;
				}
			}
			_nextStack.clear();
			_doSwitchPrepared = false;
		}

		// bool prepareNext = false;
		if (_doSwitchNext && !_transition) {
			//_doSwitchNext = false;
			if (_currentContent >= 0 && _contents[_currentContent]->useFastStop()) {
				//_contents[_currentContent]->stop();
				_contents[_currentContent]->pause();
			}
			_playCurrent = _playNext;
			int next = (_currentContent + 1) % _contents.size();
			ContentPtr nextContent = _contents[next]->get(0);
			if (nextContent && !nextContent->opened().empty()) {
				_doSwitchNext = false;
				int oldCurrent = _currentContent;
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
				_status["current-playlist-id"] = _status["next-playlist-id"];
				_status["current-index"] = Poco::format("%d", _playCurrent.i);
				_status["current-playlist"] = _status["next-playlist"];
				_status["current-content-id"] = _status["next-content-id"];
				_status["current-content"] = _status["next-content"];
				removeStatus("next-playlist-id");
				removeStatus("next-playlist");
				removeStatus("next-content-id");
				removeStatus("next-content");

				if (config().outCastLog) {
					string d = Poco::DateTimeFormatter::format(now.timestamp(), "%Y%m%d");
					if (_castLogDate != d) {
						if (_castLog) {
							_castLog->close();
							SAFE_DELETE(_castLog);
						}
						_castLogDate = d;
						File dataFile("logs/cast-" + _castLogDate + ".csv");
						bool createNewFile = !dataFile.exists();
						_castLog = new Poco::FileOutputStream();
						_castLog->open(dataFile.path(), std::ios::out | std::ios::app);
						if (createNewFile) {
							string s = "@1\r\n";
							_castLog->write(s.c_str(), s.length());
							_castLog->flush();
						}
					}
					if (_castLog) {
						string time = Poco::DateTimeFormatter::format(now.timestamp(), "%Y-%m-%d %H:%M:%S.%i");
						string s = time + "," + _status["current-content"] + "\r\n";
						_castLog->write(s.c_str(), s.length());
						_castLog->flush();
					}
				}
				_playCount++;

				Poco::ScopedLock<Poco::FastMutex> lock(_lock);
				ContentPtr currentContent = NULL;
				if (oldCurrent >= 0) {
					_contents[oldCurrent]->pause();
					currentContent = _contents[oldCurrent]->get(0);
				}
				if (currentContent) {
					SAFE_DELETE(_transition);
					if (_playCurrent.transition == "slide") {
						int h = config().stageRect.bottom;
						float speed = h / F(60); // 1s
						_transition = new SlideTransition(currentContent, nextContent, speed, 0, h);
					} else if (_playCurrent.transition == "dissolve") {
						float speed = 0.05f;
						_transition = new DissolveTransition(currentContent, nextContent, speed);
					}
					if (_transition) _transition->initialize(_frame);
				}
				if (!_transition) {
					_doPrepareNext = true;
				}
			} else {
			}
		}

		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			if (_transition && _transition->process(_frame)) {
				// トランジション終了
				SAFE_DELETE(_transition);
				// 次のコンテンツ準備
				_doPrepareNext = true;
				if (_currentContent >= 0 && _contents[_currentContent]->useFastStop()) {
					_contents[_currentContent]->stop();
				}
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
	if (now.second() != _timeSecond) {
		_timeSecond = now.second();
		_nowTime = Poco::DateTimeFormatter::format(now, "%Y/%m/%d(%w) %H:%M:%S");
		Poco::ScopedLock<Poco::FastMutex> lock(_workspaceLock);
		if (_workspace) {
			Poco::Timespan bf10(0, 0, 0, 10, 0);
			for (int i = 0; i < _workspace->getScheduleCount(); i++) {
				SchedulePtr schedule = _workspace->getSchedule(i);
				if (schedule->match(now + bf10)) {
					// 10秒前チェック
					string command = schedule->command();
					if (command.find("playlist ") == 0) {
						string params = command.substr(9);
						string playlistID = params;
						int item = 0;
						if (params.find("-") != string::npos) {
							playlistID = params.substr(0, params.find("-"));
							if (!Poco::NumberParser::tryParse(params.substr(params.find("-") + 1), item)) {
								// failed parse item no
							}
						}

						PlayListPtr playlist = _workspace->getPlaylist(playlistID);
						if (playlist) {
							_log.information(Poco::format("[%s]exec %s", _nowTime, command));
							stackPrepareContent(playlistID, item);
							drawConsole(Poco::DateTimeFormatter::format(now, "[%H:%M:%S]") + Poco::format("preparing playlist %s", playlist->id()));
						} else {
							_log.warning(Poco::format("[%s]failed command: %s", _nowTime, command));
						}
						break;
					} else if (command.find("next") == 0) {
						break;
					} else if (command.find("brightness ") == 0) {
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
							drawConsole(Poco::DateTimeFormatter::format(now, "[%H:%M:%S]") + "switched " + command);
						} else {
							_log.warning(Poco::format("[%s]failed next content not prepared %s", _nowTime, command));
							drawConsole(Poco::DateTimeFormatter::format(now, "[%H:%M:%S]") + "not switched " + command);
						}
						break;
					} else if (command.find("next") == 0) {
						_doSwitchNext = true;
						_log.information(Poco::format("[%s]exec %s", _nowTime, command));
						drawConsole(Poco::DateTimeFormatter::format(now, "[%H:%M:%S]") + "switched next");
						break;
					} else if (command.find("brightness ") == 0) {
						int brightness = -1;
						if (Poco::NumberParser::tryParse(command.substr(10), brightness) && brightness >= 0 && brightness <= 100) {
							setBrightness(brightness);
							_log.information(Poco::format("[%s]exec %s", _nowTime, command));
							drawConsole(Poco::DateTimeFormatter::format(now, "[%H:%M:%S]") + Poco::format("set brightness %d", brightness));
						}
						break;
					}
				}
			}
		}
	}

	//if (_interrupttedContent) _interrupttedContent->process(_frame);

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

	if (_brightness < 100 || config().dimmer < F(1)) {
		DWORD col = ((DWORD)(0xff * (100 - _brightness * config().dimmer) / 100) << 24) | 0x000000;
		_renderer.drawTexture(config().mainRect.left, config().mainRect.top, config().mainRect.right, config().mainRect.bottom, NULL, 0, col, col, col, col);
	}
	if (_removableCover > 0.0f) {
		DWORD col = ((DWORD)(0xff * _removableCover) << 24) | 0x000000;
		_renderer.drawTexture(config().mainRect.left, config().mainRect.top, config().mainRect.right, config().mainRect.bottom, NULL, 0, col, col, col, col);
	}
	// _renderer.drawFontTextureText(0, config().mainRect.bottom - 40, 12, 16, 0xffcccccc, Poco::format("LUMINANCE:%03d", _luminance));
}

void MainScene::draw2() {
	if (config().fullsceen && _renderer.getDisplayAdapters() > 1) {
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		LPDIRECT3DTEXTURE9 capture = _renderer.getCaptureTexture();
		if (capture) {
			D3DSURFACE_DESC desc;
			HRESULT hr = capture->GetLevelDesc(0, &desc);
			int x = config().subRect.left;
			int y = config().subRect.top;
			int w = config().subRect.right;
			int h = config().subRect.bottom - 128;
			float a1 = F(desc.Width) / desc.Height;
			float a2 = F(w) / h;
			if (a1 >= a2) {
				// 横長
				int dh = w / a1;
				y = (h - dh) / 2;
				h = dh;
			} else {
				// 縦長
				int dw = h / a1;
				x = (w - dw) / 2;
				w = dw;
			}
			device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			_renderer.drawTexture(x, y, w, h, capture, 0, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
			device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		}
	}

	string status1;
	string status2;
	if (_currentContent >= 0) {
		int current = -1;
		int duration = -1;
		for (int i = 0; i < _contents[_currentContent]->size(); ++i) {
			ContentPtr c = _contents[_currentContent]->get(i);
			if (c && !c->opened().empty()) {
				string s = c->get("status");
				if (!s.empty()) status1 = s;
				if (c->duration() > duration) {
					current = c->current();
					duration = c->duration();
					string time = c->get("time");
					status2 = Poco::format("%05d/%05d %s", current, duration, time);
					string tc = c->get("time_current");
					string tr = c->get("time_remain");
					if (!tc.empty()) _status["time_current"] = tc;
					if (!tr.empty()) _status["time_remain"] = tr;
				}
			}
		}
	}

	if (config().viewStatus && _workspace) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		_renderer.drawFontTextureText(0, config().subRect.bottom - 128, 24, 32, 0x996699ff, config().name);
		_renderer.drawTexture((config().name.size() + 1) * 24, config().subRect.bottom - 128, _description, 0, 0xcc6666ff, 0xcc6666ff, 0x993333cc, 0x993333cc);
		_renderer.drawFontTextureText(0, config().subRect.bottom - 96, 12, 16, 0x99ffffff, status1);
		_renderer.drawFontTextureText(0, config().subRect.bottom - 80, 12, 16, 0x99ccccff, "frame");
		_renderer.drawFontTextureText(144, config().subRect.bottom - 80, 12, 16, 0x99ccccff, "time");
		_renderer.drawFontTextureText(288, config().subRect.bottom - 80, 12, 16, 0x99ccccff, "remain");
		_renderer.drawFontTextureText(0, config().subRect.bottom - 64, 12, 16, 0x99ffffff, status2);
		_renderer.drawFontTextureText(0, config().subRect.bottom - 48, 12, 16, 0x99ccccff, "action");
		if (!_playCurrent.action.empty()) _renderer.drawFontTextureText(84, config().subRect.bottom - 48, 12, 16, 0x99ffffff, _playCurrent.action);
		_renderer.drawFontTextureText(264, config().subRect.bottom - 48, 12, 16, 0x99ccccff, "transition");
		if (!_playNext.transition.empty()) _renderer.drawFontTextureText(396, config().subRect.bottom - 48, 12, 16, 0x99ffffff, _playNext.transition);

		_renderer.drawFontTextureText(504, config().subRect.bottom - 128, 12, 16, 0x99ccccff, " current");
		_renderer.drawTexture(612, config().subRect.bottom - 128, _playlistName, 0, 0xccffffff, 0xccffffff,0x99ffffff, 0x99ffffff);
		_renderer.drawTexture(612, config().subRect.bottom - 112, _currentName, 0, 0xccffffff, 0xccffffff,0x99ffffff, 0x99ffffff);
		_renderer.drawFontTextureText(504, config().subRect.bottom - 96, 12, 16, 0x99ccccff, "    next");
		_renderer.drawTexture(612, config().subRect.bottom - 96, _nextName, 0, 0xccffffff, 0xccffffff,0x99ffffff, 0x99ffffff);
		_renderer.drawFontTextureText(504, config().subRect.bottom - 80, 12, 16, 0x99ccccff, "prepared");
		_renderer.drawTexture(612, config().subRect.bottom - 80, _preparedPlaylistName, 0, 0xccffffff, 0xccffffff,0x99ffffff, 0x99ffffff);
		_renderer.drawTexture(612, config().subRect.bottom - 64, _preparedName, 0, 0xccffffff, 0xccffffff,0x99ffffff, 0x99ffffff);
	}
	if (config().viewStatus) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		int next = (_currentContent + 1) % _contents.size();
		string wait(_contents[next]->opened().empty()?"preparing":"ready");
		_renderer.drawFontTextureText(0, config().subRect.bottom - 32, 12, 16, 0x99669966, Poco::format("[%s] played>%04d current>%d state>%s", _nowTime, _playCount, _currentContent, wait));
		if (!_messages.empty()) {
			if (_messageFrame > 30 && _messages.size() > 1) {
				_messageFrame = 0;
				_messages.pop();
			}
			string s = _messages.front();
			DWORD col = ((DWORD)((0x99 * (_messageFrame > 30?30:_messageFrame) / 30) << 24)) | 0xcc9900;
			_renderer.drawFontTextureText(config().subRect.right - s.length() * 12, config().subRect.bottom - 48, 12, 16, col, s);
			_messageFrame++;
		}

		//string delayedUpdate = getStatus("delayed-update");
		//if (delayedUpdate.empty()) {
		string remoteCopy = getStatus("remote-copy");
		if (!remoteCopy.empty()) {
			if (remoteCopy != "10") {
				int x = config().subRect.right - 156;
				int y = config().subRect.bottom - 16;
				DWORD col = ((_frame / 10) % 2 == 0)?0x99ff9933:0x99000000;
				_renderer.drawFontTextureText(x, y, 12, 16, col, "[copy master]");
			}
		}
		//} else {
		//	int x = config().subRect.right - 204;
		//	int y = config().subRect.bottom - 16;
		//	_renderer.drawFontTextureText(x, y, 12, 16, 0x99ff9933, "[delayedUpdating]");
		//}
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
			// _renderer.drawFontTextureText(tw, config().subRect.bottom - th / 2, 12, 16, 0xccffffff, Poco::format("%d %d(%d%%)", _currentCopySize, _copySize, _currentCopyProgress));
		}
	}
}

void MainScene::drawConsole(string s) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	_messages.push(s);
}
