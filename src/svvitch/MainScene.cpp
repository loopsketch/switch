#include "MainScene.h"

#include <Psapi.h>
#include <Poco/format.h>
#include <Poco/Exception.h>
#include <Poco/string.h>
#include <Poco/UnicodeConverter.h>
#include <Poco/Util/XMLConfiguration.h>

#include "Image.h"
#include "FFMovieContent.h"
#include "Text.h"
#include "CvContent.h"
#include "CaptureContent.h"
#include "DSContent.h"
#include "SlideTransition.h"
#include "DissolveTransition.h"


//=============================================================
// 実装
//=============================================================
//-------------------------------------------------------------
// デフォルトコンストラクタ
//-------------------------------------------------------------
MainScene::MainScene(Renderer& renderer, ui::UserInterfaceManagerPtr uim):
	Scene(renderer), _uim(uim),
	activePrepareNextMedia(this, &MainScene::prepareNextMedia),
	_workspace(NULL), _frame(0), _luminance(100), _preparing(false), _playCount(0), _transition(NULL), _interruptMedia(NULL),
	_playlistName(NULL), _currentName(NULL), _preparedName(NULL)
{
	ConfigurationPtr conf = _renderer.config();
	_luminance = conf->luminance;
	initialize();
}

//-------------------------------------------------------------
// デストラクタ
//-------------------------------------------------------------
MainScene::~MainScene() {
	_log.information("release contents");
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	for (vector<Container*>::iterator it = _contents.begin(); it != _contents.end(); it++) SAFE_DELETE(*it);
	_contents.clear();
	int  count = 50;
	while (_preparing) {
		if (count-- == 0) break;
		Sleep(100);
	}
	SAFE_DELETE(_transition);
	SAFE_DELETE(_interruptMedia);

	SAFE_RELEASE(_playlistName);
	SAFE_RELEASE(_currentName);
	SAFE_RELEASE(_preparedName);

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


//-------------------------------------------------------------
// シーンの初期化
//-------------------------------------------------------------
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
	_frame = 0;
	_log.information("*initialized MainScene");
	_startup = false;
	_log.information("*created main-scene");
	return true;
}

//-------------------------------------------------------------
// シーンを生成
// 引数
//		pD3DDevice : IDirect3DDevice9 インターフェイスへのポインタ
// 戻り値
//		成功したらS_OK
//-------------------------------------------------------------
bool MainScene::setWorkspace(WorkspacePtr workspace) {
	_workspace = workspace;
	if (_workspace->getPlaylistCount() > 0) {
		// playlistがある場合は最初のplaylistを自動スタートする
		PlayListPtr playlist = _workspace->getPlaylist(0);
		_playlistID = playlist->id();
		_playlistItem = -1;
		activePrepareNextMedia();
	} else {
		_log.warning("no playlist, no auto starting");
	}

	return true;
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
	if (!_workspace) return false;
	int count = 5;
	while (_transition || count-- > 0) {
		// トランジション中は解放しないようにする。更に初期化まで1秒くらいウェイトする
		if (_contents.empty()) return false;
		Poco::Thread::sleep(30);
	}

	// 準備フェーズ
	_preparing = true;
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
			PlayListPtr playlist = _workspace->getPlaylist(playlistID);
			if (playlist) {
				if (playlist->itemCount() > j) {
					_playlistID = playlist->id();
					_playlistItem = j;
					_log.information(Poco::format("jump playlist <%s>:%d", playlist->name(), _playlistItem));
					_playlistItem--;
				} else {
					_log.warning(Poco::format("failed jump index %d-<%d>", i, j));
				}
			} else {
				_log.warning(Poco::format("failed jump index <%d>-%d", i, j));
			}
		} else if (_currentCommand == "stop") {
			_suppressSwitch = false;
			_contents[next]->initialize();
			_preparing = false;
			return true;
		}
	}

	if (prepareMedia(_contents[next], _playlistID, _playlistItem + 1)) {
		PlayListPtr playlist = _workspace->getPlaylist(_playlistID);
		if (playlist && playlist->itemCount() > 0) {
			_playlistItem = (_playlistItem + 1) % playlist->itemCount();
			PlayListItemPtr item = playlist->items()[_playlistItem];
			_preparedCommand = item->next();
			_preparedTransition = item->transition();
			LPDIRECT3DTEXTURE9 t1 = _renderer.createTexturedText(L"", 14, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, playlist->name());
			LPDIRECT3DTEXTURE9 t2 = _renderer.createTexturedText(L"", 14, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, item->media()->name());
			{
				Poco::ScopedLock<Poco::FastMutex> lock(_lock);
				SAFE_RELEASE(_playlistName);
				_playlistName = t1;
				SAFE_RELEASE(_preparedName);
				_preparedName = t2;
			}
		}
		_suppressSwitch = false;
	} else {
		_log.warning(Poco::format("failed prepare: %s-%d", _playlistID, _playlistItem + 1));
	}
	_workspace->checkUpdate();
	_preparing = false;
	return true;
}

bool MainScene::prepareMedia(ContainerPtr container, const string& playlistID, const int i) {
//	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	_log.information(Poco::format("prepare: %s-%d", playlistID, i));
	container->initialize();
	PlayListPtr playlist = _workspace->getPlaylist(playlistID);
	if (playlist && playlist->itemCount() > 0) {
		PlayListItemPtr item = playlist->items()[i % playlist->itemCount()];
		MediaItemPtr media = item->media();
		if (media) {
			ConfigurationPtr conf = _renderer.config();
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
							movie->setPosition(conf->stageRect.left, conf->stageRect.top);
							movie->setBounds(conf->stageRect.right, conf->stageRect.bottom);
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
				for (int j = 0; j < media->fileCount(); j++) {
					TextPtr text = new Text(_renderer);
					if (text->open(media, j)) {
						container->add(text);
					} else {
						SAFE_DELETE(text);
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

void MainScene::switchContent(ContainerPtr* container, const string& playlistID, const int i) {
//	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	PlayListPtr playlist = _workspace->getPlaylist(playlistID);
	if (playlist) {
		if (playlist->itemCount() > i) {
			PlayListItemPtr item = playlist->items()[i];
			int next = (_currentContent + 1) % _contents.size();
			ContainerPtr tmp = _contents[next];
			_contents[next] = *container;
			*container = tmp;
			_log.information(Poco::format("switch content: %s-%d:%s", playlistID, i, item->media()->name()));

			LPDIRECT3DTEXTURE9 t1 = NULL;
			if (_playlistID != playlistID) {
				_playlistID = playlistID;
				t1 = _renderer.createTexturedText(L"", 14, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, playlist->name());
			}
			LPDIRECT3DTEXTURE9 t2 = _renderer.createTexturedText(L"", 14, 0xffffffff, 0xffeeeeff, 0, 0xff000000, 0, 0xff000000, item->media()->name());
			{
				Poco::ScopedLock<Poco::FastMutex> lock(_lock);
				if (t1) {
					SAFE_RELEASE(_playlistName);
					_playlistName = t1;
				}
				SAFE_RELEASE(_preparedName);
				_preparedName = t2;
			}
			_playlistItem = i;
			_preparedCommand = item->next();
			_preparedTransition = item->transition();
			_doSwitch = true;
		}
	} else {
		_log.warning(Poco::format("not find playlist: %s", playlistID));
	}
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

	if (!_startup && _frame > 200) {
		_startup = true;
		_log.information("startup auto prepare");
		int next = (_currentContent + 1) % _contents.size();
		ContentPtr nextContent = _contents[next]->get(0);
		if (nextContent && !nextContent->opened().empty()) {
			_contents[next]->play();
			{
				Poco::ScopedLock<Poco::FastMutex> lock(_lock);
				_currentName = _preparedName;
				_preparedName = NULL;
			}
			_currentCommand = _preparedCommand;
			_playCount++;
			_log.information("startup auto prepare");
//			SAFE_DELETE(_prepareNextMediaResult);
			_currentContent = next;
			activePrepareNextMedia();
			_suppressSwitch = true;
		} else {
			_log.warning("failed startup content");
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

			} else {
				if (_contents[_currentContent]->finished()) {
					_log.information(Poco::format("content[%d] finished: ", _currentContent));
					_doSwitch = true;
				}
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
					_currentName = _preparedName;
					_preparedName = NULL;
				}
				_currentCommand = _preparedCommand;
				_playCount++;

//				if (_transition) SAFE_DELETE(_transition);
				if (currentContent) {
					if (_preparedTransition == "slide") {
						const ConfigurationPtr conf = _renderer.config();
						int cw = conf->splitSize.cx;
						int ch = conf->splitSize.cy;
						_transition = new SlideTransition(currentContent, nextContent, 0, ch);
					} else if (_preparedTransition == "dissolve") {
						_transition = new DissolveTransition(currentContent, nextContent);
					}
					if (_transition) _transition->initialize(_frame);
				}
				if (!_transition) {
					_suppressSwitch = true;
					prepareNext = true;
				}
			} else {
				//
			}
		}

		if (_transition && _transition->process(_frame)) {
			// トランジション終了
			SAFE_DELETE(_transition);
			// 次のコンテンツ準備
			_suppressSwitch = true;
			prepareNext = true;
		}
		if (prepareNext) {
			try {
				activePrepareNextMedia();
			} catch (Poco::NoThreadAvailableException ex) {
			}
		}
	}

	_frame++;
}

//-------------------------------------------------------------
// オブジェクト等の描画
// 引数
//		pD3DDevice : IDirect3DDevice9 インターフェイスへのポインタ
//-------------------------------------------------------------
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

	ConfigurationPtr conf = _renderer.config();
	if (_luminance < 100) {
		DWORD col = ((DWORD)(0xff * (100 - _luminance) / 100) << 24) | 0x000000;
		_renderer.drawTexture(conf->mainRect.left, conf->mainRect.top, conf->mainRect.right, conf->mainRect.bottom, NULL, 0, col, col, col, col);
	}
	_renderer.drawFontTextureText(0, conf->mainRect.bottom - 40, 12, 16, 0xffcccccc, Poco::format("LUMINANCE:%03d", _luminance));
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

	if (_currentContent >= 0) {
		ContentPtr c = _contents[_currentContent]->get(0);
		if (c && !c->opened().empty()) {
			MediaItemPtr media = _workspace->getMedia(c->opened());
			int current = c->current();
			int duration = c->duration();
			string time;
			if (media->type() == MediaTypeMovie) {
				FFMovieContentPtr movie = dynamic_cast<FFMovieContentPtr>(c);
				DWORD currentTime = movie->currentTime();
				DWORD timeLeft = movie->timeLeft();
				Uint32 fps = movie->getFPS();
				float avgTime = movie->getAvgTime();
				time = Poco::format("%02lu:%02lu.%03lu %02lu:%02lu.%03lu", currentTime / 60000, currentTime / 1000 % 60, currentTime % 1000 , timeLeft / 60000, timeLeft / 1000 % 60, timeLeft % 1000);
				LPDIRECT3DTEXTURE9 name = _renderer.getCachedTexture(media->id());
				_renderer.drawTexture(0, 580, name, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
				_renderer.drawFontTextureText(0, 595, 12, 16, 0xccffffff, Poco::format("%03lufps(%03.2hfms)", fps, avgTime));
			} else {
				LPDIRECT3DTEXTURE9 name = _renderer.getCachedTexture(media->id());
				_renderer.drawTexture(0, 580, name, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
			}
			_renderer.drawFontTextureText(0, 610, 12, 16, 0xccffffff, Poco::format("%04d/%04d %s", current, duration, time));
		}
	}
	if (!_currentCommand.empty()) {
		_renderer.drawFontTextureText(0, 625, 12, 16, 0xccffffff, Poco::format(">%s", _currentCommand));
	}

	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		int next = (_currentContent + 1) % _contents.size();
//		FFMovieContentPtr nextMovie = dynamic_cast<FFMovieContentPtr>(_contents[next]->get(0));
//		FFMovieContentPtr nextMovie = (FFMovieContentPtr)_contents[next]->get(0);
		string wait(_contents[next]->opened().empty()?"preparing":"ready");
		_renderer.drawFontTextureText(0, 700, 12, 16, 0xccffffff, Poco::format("play contents:%04d playing no<%d> next:%s", _playCount, _currentContent, wait));

		_renderer.drawTexture(0, 640, _playlistName, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
		_renderer.drawTexture(0, 655, _currentName, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
		_renderer.drawTexture(0, 670, _preparedName, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
		if (!_currentCommand.empty()) _renderer.drawFont(0, 685, 0xffffff, 0x000000, Poco::format("next>%s", _currentCommand));
		if (!_preparedTransition.empty()) _renderer.drawFont(0, 700, 0xffffff, 0x000000, Poco::format("transition>%s", _preparedTransition));
//		if (_prepareNextMediaResult) {
//			string available(_prepareNextMediaResult->available()?"available":"not available");
//			_renderer.drawFont(500, 700, 0xffffff, 0x000000, Poco::format("prepare result %s", available));			
//		}
	}
}

