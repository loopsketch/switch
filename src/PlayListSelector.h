#pragma once

#include <Poco/HashMap.h>
#include <Poco/Mutex.h>

#include "UI.h"
#include "Renderer.h"
#include "Workspace.h"

using std::vector;


class PlayListSelector: public Content
{
private:
	Poco::FastMutex _lock;

	WorkspacePtr _workspace;
	int _playlist;
	int _item;

	int _oldPlaylist;
	float _changePlaylist;
	float _moveSpeed;
	int _changePlaylistItem;

public:
	PlayListSelector(Renderer& renderer);
	virtual ~PlayListSelector(void);

	void update(Workspace* workspace);

	int getPlayList();
	int getPlayListItem();

	void nextPlayList();
	void beforePlayList();
	void nextPlayListItem();
	void beforePlayListItem();

	void draw(const DWORD& frame);
};
