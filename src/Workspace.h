#pragma once

#include <vector>
#include <Poco/HashMap.h>
#include <Poco/Logger.h>

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

	Renderer& _renderer;
	Poco::HashMap<string, MediaItemPtr> _media;
	vector<PlayListPtr> _playlists;

	void initialize();

public:
	Workspace(Renderer& renderer);

	~Workspace();

	/** XMLÇÉpÅ[ÉX */
	bool parse(const string file);

	void release();

	const int getPlayListCount() const;

	const PlayListPtr getPlayList(int i) const;

	PlayListItemPtr prepareMedia(ContainerPtr container, PlayListPtr playlist, const int itemIndex);
};

typedef Workspace* WorkspacePtr;
