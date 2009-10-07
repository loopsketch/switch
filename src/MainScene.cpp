#include "MainScene.h"

#include <Psapi.h>
#include <Poco/format.h>
#include <Poco/Exception.h>
#include <Poco/string.h>
#include <Poco/UnicodeConverter.h>
#include <Poco/Util/XMLConfiguration.h>

#include "Image.h"
#include "Movie.h"
#include "Text.h"
#include "SlideTransition.h"
#include "DissolveTransition.h"


//=============================================================
// 実装
//=============================================================
//-------------------------------------------------------------
// デフォルトコンストラクタ
//-------------------------------------------------------------
MainScene::MainScene(Renderer& renderer):
	Scene(renderer), activeCloseNextMedia(this, &MainScene::closeNextMedia), activePrepareNextMedia(this, &MainScene::prepareNextMedia),
	_workspace(NULL), _frame(0), _luminance(100), _playCount(0), _transition(NULL), _currentPlaylist(NULL), _nextItem(NULL), _interruptMedia(NULL)
{
	ConfigurationPtr conf = _renderer.config();
	_luminance = conf->luminance;
	initialize();
}

//-------------------------------------------------------------
// デストラクタ
//-------------------------------------------------------------
MainScene::~MainScene() {
	for (vector<Container*>::iterator it = _contents.begin(); it != _contents.end(); it++) SAFE_DELETE(*it);
	_contents.clear();
	SAFE_DELETE(_transition);
	SAFE_DELETE(_interruptMedia);
	try {
		Poco::Util::XMLConfiguration* xml = new Poco::Util::XMLConfiguration("switch-config.xml");
		if (xml) {
			xml->setInt("display.stage.luminnace", _luminance);
			xml->save("switch-config.xml");
			xml->release();
		}
	} catch (Poco::Exception& ex) {
		_log.warning(ex.displayText());
	}
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
	return true;
}

//-------------------------------------------------------------
// シーンを生成
// 引数
//		pD3DDevice : IDirect3DDevice9 インターフェイスへのポインタ
// 戻り値
//		成功したらS_OK
//-------------------------------------------------------------
bool MainScene::setWorkspace(WorkspacePtr workspace)
{
	_workspace = workspace;
	if (_workspace->getPlayListCount() > 0) {
		_currentItem = -1;
		_currentPlaylist = _workspace->getPlayList(0);
		activePrepareNextMedia();
	} else {
		_log.warning("no playlist, no auto starting");
	}
	_startup = false;
	_log.information("*created main-scene");
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

void MainScene::closeNextMedia() {
	int count = 10;
	while (_transition || count-- > 0) {
		// トランジション中は解放しないようにする。更に次のコンテンツの初期化まで1秒くらいウェイトする
		if (_contents.empty()) return;
		Poco::Thread::sleep(30);
	}
	int next = (_currentContent + 1) % _contents.size();
	_contents[next]->initialize();
}

void MainScene::prepareNextMedia() {
	// 初期化フェーズ
	int count = 5;
	while (_transition || count-- > 0) {
		// トランジション中は解放しないようにする。更に初期化まで1秒くらいウェイトする
		if (_contents.empty()) return;
		Poco::Thread::sleep(30);
	}

	// 準備フェーズ
	int next = (_currentContent + 1) % _contents.size();
	_contents[next]->initialize();
	{
//		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		if (!_currentCommand.empty()) {
			int jump = _currentCommand.find_first_of("jump");
			if (jump == 0) {
				string s = _currentCommand.substr(4);
				int i = Poco::NumberParser::parse(s.substr(0, s.find("-")));
				int j = Poco::NumberParser::parse(s.substr(s.find("-") + 1));
				PlayListPtr playlist = _workspace->getPlayList(i);
				if (playlist) {
					if (playlist->itemCount() > j) {
						_currentPlaylist = playlist;
						_currentItem = j;
						_log.information(Poco::format("jump playlist <%s>:%d", _currentPlaylist->name(), _currentItem));
						_currentItem--;
					} else {
						_log.warning(Poco::format("failed jump index %d-<%d>", i, j));
					}
				} else {
					_log.warning(Poco::format("failed jump index <%d>-%d", i, j));
				}
			} else if (_currentCommand == "stop") {
				_suppressSwitch = false;
				return;
			}
		}
		_currentItem = (_currentItem + 1) % _currentPlaylist->itemCount();
		_nextItem = _workspace->prepareMedia(_contents[next], _currentPlaylist, _currentItem);
		_suppressSwitch = false;
	}
}

void MainScene::switchContent(ContainerPtr* container, PlayListPtr playlist, const int i) {
//	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	_currentPlaylist = playlist;
	_currentItem = i;
	_nextItem = _currentPlaylist->items()[_currentItem];
	int next = (_currentContent + 1) % _contents.size();
	ContainerPtr tmp = _contents[next];
	_contents[next] = *container;
	*container = tmp;
	_doSwitch = true;
	_log.information("main scene switchContent");
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

//	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (!_startup && _frame > 100) {
		_startup = true;
		_currentContent = (_currentContent + 1) % _contents.size();
		_contents[_currentContent]->play();
		if (_nextItem) _currentCommand = _nextItem->next();
		_playCount++;
		_suppressSwitch = true;
		activePrepareNextMedia();
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
					_doSwitch = true;
				}
			}
		}

		bool prepareNext = false;
		if (_doSwitch && !_transition) {
			_doSwitch = false;
			int next = (_currentContent + 1) % _contents.size();
			ContentPtr nextContent = _contents[next]->get(0);
			if (nextContent && nextContent->opened()) {
				_currentContent = next;
				_contents[next]->play();
				if (_nextItem) _currentCommand = _nextItem->next();
				_playCount++;

//				if (_transition) SAFE_DELETE(_transition);
				if (currentContent) {
					if (_nextItem) {
						if (_nextItem->transition() == "slide") {
							const ConfigurationPtr conf = _renderer.config();
							int cw = conf->splitSize.cx;
							int ch = conf->splitSize.cy;
							_transition = new SlideTransition(currentContent, nextContent, 0, ch);
						} else if (_nextItem->transition() == "dissolve") {
							_transition = new DissolveTransition(currentContent, nextContent);
						}
					}
					if (_transition) _transition->initialize(_frame);
				}
				if (!_transition) {
					_suppressSwitch = true;
					prepareNext = true;
				}
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
			activePrepareNextMedia();
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
		_renderer.drawTexture(conf->mainRect.left, conf->mainRect.top, conf->mainRect.right, conf->mainRect.bottom, NULL, col, col, col, col);
	}

	_renderer.drawFontTextureText(0, conf->mainRect.bottom - 40, 12, 16, 0xffcccccc, Poco::format("LUMINANCE:%03d", _luminance));
}

void MainScene::draw2() {
	if (_renderer.getDisplayAdapters() > 1) {
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		LPDIRECT3DTEXTURE9 capture = _renderer.getCaptureTexture();
		_renderer.drawTexture(0, 50, 256, 192, capture, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	}

	if (_currentContent >= 0) {
		ContentPtr c = _contents[_currentContent]->get(0);
		if (c && c->opened()) {
			MediaItemPtr media = c->opened();
			int current = c->current();
			int duration = c->duration();
			string time;
			if (media->type() == MediaTypeMovie) {
				MoviePtr movie = (MoviePtr)c;
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
//		MoviePtr nextMovie = dynamic_cast<MoviePtr>(_contents[next]->get(0));
//		MoviePtr nextMovie = (MoviePtr)_contents[next]->get(0);
		string wait(_contents[next]->opened()?"ready":"preparing");
		_renderer.drawFontTextureText(0, 700, 12, 16, 0xccffffff, Poco::format("play contents:%04d playing no<%d> next:%s", _playCount, _currentContent, wait));
		if (_nextItem) {
			const MediaItemPtr media = _nextItem->media();
			if (media && _currentPlaylist) {
				LPDIRECT3DTEXTURE9 playlist = _renderer.getCachedTexture(_currentPlaylist->name());
				_renderer.drawTexture(0, 640, playlist, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
				LPDIRECT3DTEXTURE9 name = _renderer.getCachedTexture(media->id());
				_renderer.drawTexture(0, 655, name, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
				if (!_nextItem->next().empty()) _renderer.drawFontTextureText(0, 670, 12, 16, 0xccffffff, Poco::format("next>%s", _nextItem->next()));
				if (!_nextItem->transition().empty()) _renderer.drawFontTextureText(0, 685, 12, 16, 0xccffffff, Poco::format("transition>%s", _nextItem->transition()));
			}
		}
	}
}

