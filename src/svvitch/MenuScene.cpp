#include "MenuScene.h"

MenuScene::MenuScene(Renderer& renderer, ui::UserInterfaceManagerPtr uim):
	Scene(renderer), _uim(uim)
{
}

//-------------------------------------------------------------
// デストラクタ
//-------------------------------------------------------------
MenuScene::~MenuScene() {
	_log.information("*release menu-scene");
}

bool MenuScene::initialize() {
	return true;
}

void MenuScene::process() {
}

void MenuScene::draw1() {
}

void MenuScene::draw2() {
	_renderer.drawFontTextureText(800, 0, 16, 20, 0xccffffff,"MENU SCENE");
}
