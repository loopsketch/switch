#pragma once

#include <vector>
#include <Poco/HashMap.h>
#include <Poco/Logger.h>
#include <Poco/Mutex.h>
#include <Poco/Path.h>

#include "Container.h"
#include "MediaItem.h"
#include "PlayList.h"
#include "Schedule.h"
#include "Renderer.h"

using std::string;
using std::vector;


/**
 * ワークススペースクラス.
 * サイネージの運営に必要なデータ郡を表します
 */
class Workspace
{
private:
	Poco::Logger& _log;

	Poco::FastMutex _lock;

	Path _file;
	string _signature;

	vector<MediaItemPtr> _media;
	Poco::HashMap<string, MediaItemPtr> _mediaMap;
	vector<string> _existsFiles;

	vector<PlayListPtr> _playlist;
	Poco::HashMap<string, PlayListPtr> _playlistMap;

	vector<string> _fonts;

	vector<SchedulePtr> _schedule;

	void release();

public:
	Workspace(Path file);

	~Workspace();

	const Path& file() const;

	bool parse();

	bool checkUpdate();

	const int getMediaCount();

	const MediaItemPtr getMedia(int i);

	const MediaItemPtr getMedia(string id);

	const int getPlaylistCount();

	const PlayListPtr getPlaylist(int i);

	const PlayListPtr getPlaylist(string id);

	const vector<string> getFonts();

	const int getScheduleCount();

	const SchedulePtr getSchedule(int i);

	const vector<string> existsFiles();

	const string signature() const;
};


typedef Workspace* WorkspacePtr;
