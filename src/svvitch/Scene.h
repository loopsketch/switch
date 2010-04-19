#pragma once

#include "Renderer.h"
#include "Workspace.h"
#include <string>
#include <map>

using std::string;
using std::map;


class Scene
{
protected:
	Poco::Logger& _log;
	Renderer& _renderer;
	bool _visible;
	int _keycode;
	bool _shift;
	bool _ctrl;

	/** �X�e�[�^�X�p�̃}�b�v */
	map<string, string> _status;

public:
	Scene(Renderer& renderer);

	virtual ~Scene();

	virtual bool initialize();

	/** �\��/��\���̐ݒ� */
	void setVisible(const bool visible);

	virtual void notifyKey(const int keycode, const bool shift, const bool ctrl);

	/** �X�e�[�^�X���̐ݒ� */
	virtual void setStatus(const string& key, const string& value);

	/** �X�e�[�^�X���̎擾 */
	virtual const map<string, string>& getStatus();

	/** �X�e�[�^�X���̎擾 */
	virtual const string getStatus(const string& key);

	/** �X�e�[�^�X���̍폜 */
	virtual void removeStatus(const string& key);

	virtual void process();

	virtual void draw1();

	virtual void draw2();
};

typedef Scene* ScenePtr;