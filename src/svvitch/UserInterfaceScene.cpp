#include "UserInterfaceScene.h"


UserInterfaceScene::UserInterfaceScene(Renderer& renderer, ui::UserInterfaceManagerPtr uim):
	Scene(renderer), _uim(uim)
{
}

UserInterfaceScene::~UserInterfaceScene() {	
	_log.information("*release userinterface-scene");
}

void UserInterfaceScene::process() {
	_uim->process(_frame);
	_frame++;
}

void UserInterfaceScene::draw1() {
}

void UserInterfaceScene::draw2() {
	_uim->draw(_frame);
}
