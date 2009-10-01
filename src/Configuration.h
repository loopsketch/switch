#pragma once

#include <string>

using std::string;
using std::wstring;


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
	string vpCommandFile;
	string monitorFile;
	string workspaceFile;
	string newsURL;
};

typedef Configuration* ConfigurationPtr;