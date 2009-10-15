#pragma once

#include <vector>
#include <Poco/HashMap.h>
#include <Poco/Logger.h>
#include <Poco/Mutex.h>

#include "Container.h"
#include "MediaItem.h"
#include "PlayList.h"
#include "Renderer.h"

using std::string;
using std::vector;


class Workspace
{
private:
	Poco::Logger& _log;

	Poco::FastMutex _lock;

	string _file;
	string _signature;

	vector<MediaItemPtr> _media;
	Poco::HashMap<string, MediaItemPtr> _mediaMap;

	vector<PlayListPtr> _playlist;
	Poco::HashMap<string, PlayListPtr> _playlistMap;

	void release();

public:
	Workspace(string file);

	~Workspace();

	bool parse();

	bool checkUpdate();

	const int getMediaCount();

	const MediaItemPtr getMedia(int i);

	const MediaItemPtr getMedia(string id);


	const int getPlaylistCount();

	const PlayListPtr getPlaylist(int i);

	const PlayListPtr getPlaylist(string id);
};


typedef Workspace* WorkspacePtr;
