#include "Scene.h"


Scene::Scene(Renderer& renderer): _log(Poco::Logger::get("")), _renderer(renderer)
{
}

Scene::~Scene() {
}


void Scene::initialize() {
}

HRESULT Scene::create(WorkspacePtr workspace) {
	return S_OK;
}

void Scene::notifyKey(const int keycode, const bool shift, const bool ctrl) {
	_keycode = keycode;
	_shift = shift;
	_ctrl = ctrl;
}

void Scene::process() {
}

void Scene::draw1() {
}

void Scene::draw2() {
}
