#include "Scene.h"


Scene::Scene(Renderer& renderer): _log(Poco::Logger::get("")), _renderer(renderer), _visible(true)
{
}

Scene::~Scene() {
}


bool Scene::initialize() {
	return false;
}

void Scene::setVisible(const bool visible) {
	_visible = visible;
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

const string Scene::getStatus(const string& key) {
	map<string, string>::const_iterator it = _status.find(key);
	if (it != _status.end()) {
		return it->second;
	}
	return string("");
}

void Scene::removeStatus(const string& key) {
	map<string, string>::const_iterator it = _status.find(key);
	if (it != _status.end()) _status.erase(it);
}

void Scene::process() {
}

void Scene::draw1() {
}

void Scene::draw2() {
}
