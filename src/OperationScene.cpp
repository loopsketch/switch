#include "OperationScene.h"

#include <Psapi.h>
#include <Poco/format.h>
#include <Poco/Exception.h>
#include <Poco/string.h>
#include <Poco/UnicodeConverter.h>

#include "Image.h"
#include "MainScene.h"
#include "Movie.h"
#include "Text.h"
#include "SlideTransition.h"
#include "DissolveTransition.h"
#include "ui/MouseListener.h"
#include "ui/SelectedListener.h"


//=============================================================
// 実装
//=============================================================
//-------------------------------------------------------------
// デフォルトコンストラクタ
//-------------------------------------------------------------
OperationScene::OperationScene(Renderer& renderer, ui::UserInterfaceManagerPtr uim):
	Scene(renderer), activeUpdateContentList(this, &OperationScene::updateContentList), activePrepareContent(this, &OperationScene::prepareContent),
	_uim(uim), _workspace(NULL), _frame(0), _interruptMedia(NULL), _prepareUpdate(false), _prepared(false), _prepareStart(0)
{
	initialize();
}

//-------------------------------------------------------------
// デストラクタ
//-------------------------------------------------------------
OperationScene::~OperationScene() {
	for (vector<Container*>::iterator it = _contents.begin(); it != _contents.end(); it++) SAFE_DELETE(*it);
	SAFE_DELETE(_interruptMedia);
	SAFE_DELETE(_contentsSelect);
	SAFE_DELETE(_playListSelect);
	SAFE_DELETE(_switchButton);
}


//-------------------------------------------------------------
// シーンの初期化
//-------------------------------------------------------------
bool OperationScene::initialize() {
	_playListSelect = new ui::SelectList("playlist", _uim, 300, 100, 300, 400);
	class PlaylistSelected: public ui::SelectedListener {
		friend class OperationScene;
		OperationScene& _scene;
		PlaylistSelected(OperationScene& scene): _scene(scene)  {
		}

		void selected() {
			try {
				_scene.activeUpdateContentList();
			} catch (Poco::NoThreadAvailableException& ex) {
				_log.warning(Poco::format("failed activeUpdateContentList: %s", ex.displayText()));
			}
		}
	};
	_playListSelect->setSelectedListener(new PlaylistSelected(*this));
	_contentsSelect = new ui::SelectList("contentslist", _uim, 620, 100, 400, 200);
	class ContentSelected: public ui::SelectedListener {
		friend class OperationScene;
		OperationScene& _scene;
		ContentSelected(OperationScene& scene): _scene(scene) {
		}

		void selected() {
			_scene.updatePrepare();
		}
	};
	_contentsSelect->setSelectedListener(new ContentSelected(*this));

	_switchButton = new ui::Button("switch", _uim, 880, 50, 140, 40);
	_switchButton->setBackground(0xffff0000);
	_switchButton->setText("SWITCH");
	class SwitchMouseListener: public ui::MouseListener {
		friend class OperationScene;
		OperationScene& _scene;
		SwitchMouseListener(OperationScene& scene): _scene(scene) {
		}

		void buttonDownL() {
			_scene.switchContent();
		}
	};
	_switchButton->setMouseListener(new SwitchMouseListener(*this));

	_prepareStart = 0;
	for (vector<Container*>::iterator it = _contents.begin(); it != _contents.end(); it++) SAFE_DELETE(*it);
	_contents.push_back(new Container(_renderer));
	_contents.push_back(new Container(_renderer)); // 2個のContainer
	_currentContent = -1;

	_frame = 0;

	_log.information("*initialized OperationScene");
	return true;
}

//-------------------------------------------------------------
// シーンを生成
// 引数
//		pD3DDevice : IDirect3DDevice9 インターフェイスへのポインタ
// 戻り値
//		成功したらS_OK
//-------------------------------------------------------------
bool OperationScene::setWorkspace(WorkspacePtr workspace)
{
	_workspace = workspace;
	LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
	if (device == 0) return false;

	_playListSelect->removeAll();
	for (int i = 0; i < _workspace->getPlayListCount(); i++) {
		PlayListPtr playlist = _workspace->getPlayList(i);
		LPDIRECT3DTEXTURE9 texture = _renderer.getCachedTexture(playlist->name());
		_playListSelect->addItem(texture);
	}
	if (_workspace->getPlayListCount() > 0) _playListSelect->setSelected(0);
	_log.information("*created operation-scene");
	return true;
}


void OperationScene::updateContentList() {
//	_log.information("scene updateContentList");
//	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	_contentsSelect->removeAll();
	int selected = _playListSelect->getSelectedIndex();
	if (selected >= 0) {
		PlayListPtr playlist = _workspace->getPlayList(selected);
		if (playlist) {
			vector<PlayListItemPtr> items = playlist->items();
			for (int i = 0; i < playlist->itemCount(); i++) {
				PlayListPtr playlist = _workspace->getPlayList(i);
				LPDIRECT3DTEXTURE9 texture = _renderer.getCachedTexture(items[i]->media()->id());
				_contentsSelect->addItem(texture);
			}
			_contentsSelect->setSelected(0);
		}
	}
}

void OperationScene::updatePrepare() {
	_prepareUpdate = true;
}

void OperationScene::prepareContent() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	PlayListPtr playlist = _workspace->getPlayList(_playListSelect->getSelectedIndex());
	if (playlist) {
		int next = (_currentContent + 1)  % _contents.size();
		_contents[next]->initialize();
		PlayListItemPtr res = _workspace->prepareMedia(_contents[next], playlist, _contentsSelect->getSelectedIndex());
		if (res) {
			_prepared = true;
			_contents[next]->setProperty("prepare", "true");
		}
	}
}

void OperationScene::switchContent() {
	MainScenePtr scene = dynamic_cast<MainScenePtr>(_renderer.getScene("main"));
	if (scene) {
		PlayListPtr playlist = _workspace->getPlayList(_playListSelect->getSelectedIndex());
		if (playlist) {
			updatePrepare();
			_contents[_currentContent]->setProperty("prepare", "false");
			scene->switchContent(&_contents[_currentContent], playlist, _contentsSelect->getSelectedIndex());
		}
	}
}

void OperationScene::prepareInterruptFile(string file) {
	SAFE_DELETE(_interruptMedia);
	vector<MediaItemFilePtr> files;
	files.push_back(new MediaItemFile(MediaTypeMovie, file, ""));
	_interruptMedia = new MediaItem(MediaTypeMovie, file, file, 0, files);
	int next = (_currentContent + 1)  % _contents.size();
	_contents[next]->initialize();

	MoviePtr movie = new Movie(_renderer);
	movie->open(_interruptMedia);
	_contents[next]->add(movie);
	_contents[next]->play();
	_log.information(Poco::format("interrupt file: %s", file));
}

void OperationScene::process() {
	ContentPtr currentContent = NULL;
	if (_currentContent >= 0) {
		currentContent = _contents[_currentContent]->get(0);
		_contents[_currentContent]->process(_frame);
	}

	bool availableSplitMode = (_renderer.config()->splitSize.cx > 0 && _renderer.config()->splitSize.cy > 0);
	switch (_keycode) {
		case VK_LEFT:
			_contentsSelect->selectBefore();
			break;
		case VK_RIGHT:
			_contentsSelect->selectNext();
			break;
		case VK_UP:
			_playListSelect->selectBefore();
			break;
		case VK_DOWN:
			_playListSelect->selectNext();
			break;

		case VK_RETURN:
		case VK_SPACE:
			if (_switchButton->getEnabled()) switchContent();
			break;

		case '1':
			if (availableSplitMode) _renderer.config()->splitType = 0;
			break;
		case '2':
			if (availableSplitMode) _renderer.config()->splitType = 1;
			break;
		case '3':
			if (availableSplitMode) _renderer.config()->splitType = 2;
			break;

		case 'M':
			if (currentContent) {
				if (currentContent->playing()) {
					_contents[_currentContent]->stop();
				} else {
					_contents[_currentContent]->play();
				}
			}
			break;

		case VK_F1:
			break;

		case VK_F2:
			break;
	}

	if (_prepared) {
		_prepared = false;
		_currentContent = (_currentContent + 1) % _contents.size();
		_switchButton->setEnabled(true);
	}
	if (_prepareUpdate) {
		_prepareUpdate = false;
		_switchButton->setEnabled(false);
		_prepareStart = _frame + 1;
	}
	if (_prepareStart > 0 && (long)(_frame - _prepareStart) > 30) {
		_prepareStart = 0;
		try {
			activePrepareContent();
		} catch (Poco::NoThreadAvailableException& ex) {
			_log.warning(Poco::format("failed activePrepareContent: %s", ex.displayText()));
		}
	}

	_frame++;
}

//-------------------------------------------------------------
// オブジェクト等の描画
// 引数
//		pD3DDevice : IDirect3DDevice9 インターフェイスへのポインタ
//-------------------------------------------------------------
void OperationScene::draw1() {
}

void OperationScene::draw2() {
	if (_currentContent >= 0 && _prepareStart == 0) {
		ContentPtr c = _contents[_currentContent]->get(0);
		if (c && c->opened()) {
			MediaItemPtr media = c->opened();
			LPDIRECT3DTEXTURE9 name = _renderer.getCachedTexture(media->id());
			_renderer.drawTexture(700, 580, name, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
			_contents[_currentContent]->draw(_frame);
		}
	}
	_uim->draw(_frame);
}

