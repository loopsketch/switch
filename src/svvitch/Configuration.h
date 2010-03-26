#pragma once

#include <string>
#include <vector>
#include <Poco/Path.h>

using std::string;
using std::wstring;
using Poco::Path;


class Configuration
{
public:
	string windowTitle;
	string name;
	RECT mainRect;
	int mainRate;
	RECT subRect;
	int subRate;
	DWORD frameIntervals;
	bool frame;
	bool fullsceen;

	bool useClip;
	RECT clipRect;
	RECT stageRect;
	int splitType;
	SIZE splitSize;
	int splitCycles;

	string useScenes;

	int brightness;
	bool viewStatus;
	int imageSplitWidth;
	string textFont;
	Gdiplus::FontStyle textStyle;
	int textHeight;

	bool mouse;
	bool draggable;
	wstring defaultFont;
	string asciiFont;
	string multiByteFont;
//	string vpCommandFile;
//	string monitorFile;
	Path dataRoot;
	Path workspaceFile;
	string newsURL;

	int serverPort;
	int maxQueued;
	int maxThreads;
};

typedef Configuration* ConfigurationPtr;