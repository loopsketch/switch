#pragma once

#include <Poco/HashMap.h>
#include <Poco/Mutex.h>

#include "ui/UserInterfaceManager.h"
#include "ui/Component.h"

#include "Workspace.h"

using std::vector;


namespace ui {
	class PlayListSelector: public Component
	{
	private:
		Poco::FastMutex _lock;

		Workspace* _workspace;
		int _playlist;
		int _item;

		int _oldPlaylist;
		float _changePlaylist;
		float _moveSpeed;
		int _changePlaylistItem;

	public:
		PlayListSelector(std::string name, UserInterfaceManager* uim, int x, int y, int w, int h);
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
}
