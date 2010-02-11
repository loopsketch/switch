#include "Scene.h"


Scene::Scene(Renderer& renderer): _log(Poco::Logger::get("")), _renderer(renderer)
{
}

Scene::~Scene() {
}


bool Scene::initialize() {
	return false;
}

void Scene::notifyKey(const int keycode, const bool shift, const bool ctrl) {
	_keycode = keycode;
	_shift = shift;
	_ctrl = ctrl;
}

void Scene::setStatus(const string& key, const string& value) {
	_status[key] = value;
}

const map<string, string>& Scene::getStatus() {
	return _status;
}

void Scene::process() {
}

void Scene::draw1() {
}

void Scene::draw2() {
}
