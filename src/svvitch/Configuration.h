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
	string title;
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

	string useScenes;

	int luminance;
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
	string workspaceFile;
	string newsURL;
};

typedef Configuration* ConfigurationPtr;