#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <Poco/Path.h>
#include <Poco/Channel.h>
#include <Poco/Logger.h>

using std::string;
using std::wstring;
using std::vector;
using Poco::Path;


class Configuration
{
private:
	Poco::Logger& _log;

public:
	Poco::Channel* logFile;

	string windowTitle;
	string name;
	string description;
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
	float captureQuality;
	string captureFilter;
	SIZE splitSize;
	int splitCycles;

	vector<string> movieEngines;
	vector<string> scenes;

	int brightness;
	float dimmer;
	bool viewStatus;
	int imageSplitWidth;
	string textFont;
	string textStyle;
	int textHeight;

	bool mouse;
	bool draggable;
	wstring defaultFont;
	string asciiFont;
	string multiByteFont;
	// string vpCommandFile;
	// string monitorFile;
	Path dataRoot;
	Path stockRoot;
	Path workspaceFile;
	string newsURL;

	int serverPort;
	int maxQueued;
	int maxThreads;

	bool outCastLog;


	Configuration();

	virtual ~Configuration();

	bool initialize();

	void save();

	bool hasScene(string s);

	void release();
};

typedef Configuration* ConfigurationPtr;