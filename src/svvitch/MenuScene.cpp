#include "MenuScene.h"
#include "MainScene.h"


MenuScene::MenuScene(Renderer& renderer, ui::UserInterfaceManagerPtr uim):
	Scene(renderer), _uim(uim), _goOperation(NULL), _goEdit(NULL), _goSetup(NULL)
{
}

//-------------------------------------------------------------
// デストラクタ
//-------------------------------------------------------------
MenuScene::~MenuScene() {
	SAFE_DELETE(_goOperation);
	SAFE_DELETE(_goEdit);
	SAFE_DELETE(_goSetup);
	_log.information("*release menu-scene");
}

bool MenuScene::initialize() {
	MainScenePtr mainScene = dynamic_cast<MainScenePtr>(_renderer.getScene("main"));
	if (mainScene) {
		_goOperation = new ui::Button("gooperation", _uim, 600, 100, 300, 150);
		_goOperation->setBackground(0xff333333);
		_goOperation->setText("Operation Mode");
		class GoOperationMouseListener: public ui::MouseListener {
			friend class MainScene;
			friend class MenuScene;
			MainScene& _scene;
			GoOperationMouseListener(MainScene& scene): _scene(scene) {
			}

			void buttonDownL() {
				_scene.activeGoOperation();
			}
		};
		_goOperation->setMouseListener(new GoOperationMouseListener(*mainScene));

		_goEdit = new ui::Button("goedit", _uim, 600, 260, 300, 150);
		_goEdit->setBackground(0xff333333);
		_goEdit->setText("Edit Mode");
		class GoEditMouseListener: public ui::MouseListener {
			friend class MainScene;
			friend class MenuScene;
			MainScene& _scene;
			GoEditMouseListener(MainScene& scene): _scene(scene) {
			}

			void buttonDownL() {
				_scene.activeGoEdit();
			}
		};
		_goEdit->setMouseListener(new GoEditMouseListener(*mainScene));

		_goSetup = new ui::Button("gosetup", _uim, 600, 420, 300, 150);
		_goSetup->setBackground(0xff333333);
		_goSetup->setText("Setup Mode");
		class GoSetupMouseListener: public ui::MouseListener {
			friend class MainScene;
			friend class MenuScene;
			MainScene& _scene;
			GoSetupMouseListener(MainScene& scene): _scene(scene) {
			}

			void buttonDownL() {
				_scene.activeGoSetup();
			}
		};
		_goSetup->setMouseListener(new GoSetupMouseListener(*mainScene));

		return true;
	} else {
		_log.warning("not found main-scene");
	}
	return false;
}


void MenuScene::process() {
}

void MenuScene::draw1() {
}

void MenuScene::draw2() {
	_renderer.drawFontTextureText(800, 0, 16, 20, 0xccffffff,"MENU SCENE");
}
