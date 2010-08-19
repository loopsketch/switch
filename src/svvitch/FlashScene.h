#pragma once

#ifdef UNICODE
#define FormatMessage FormatMessageW
#define FindResource FindResourceW
#define GetModuleFileName GetModuleFileNameW
#define CreateFile CreateFileW
#define LoadLibrary LoadLibraryW
#define CreateEvent CreateEventW
#else
#define FormatMessage FormatMessageA
#define FindResource FindResourceA
#define GetModuleFileName GetModuleFileNameA
#define CreateFile CreateFileA
#define LoadLibrary LoadLibraryA
#define CreateEvent CreateEventA
#endif // !UNICODE

#include <windows.h>
#include <atlbase.h>
#include <string>
#include "Scene.h"

using std::string;
using std::wstring;

// Shockwave Flash ActiveX interfaces

struct IOleObject;
struct IOleInPlaceObjectWindowless;

namespace ShockwaveFlashObjects
{
	struct IShockwaveFlash;
}


class FlashListener
{
public:
	virtual void			FlashAnimEnded() {}
	virtual void			FlashCommand(const std::string& theCommand, const std::string& theParam) {}
};

// Forward Declarations
class ControlSite;


class FlashScene: public Scene
{
private:
	HMODULE _module;
	ControlSite* _controlSite;
	IOleObject* _ole;
	ShockwaveFlashObjects::IShockwaveFlash* _flash;
	IOleInPlaceObjectWindowless* _windowless;
	IViewObject* _view;

	LPDIRECT3DTEXTURE9 _buf;

public:
	FlashScene(Renderer& renderer);

	virtual ~FlashScene();

	virtual bool initialize();

	virtual void process();

	virtual void draw1();

	virtual void draw2();

	long getReadyState();

	bool loadMovie(const std::string& file);

	bool isPlaying();

	int getCurrentFrame();
};

typedef FlashScene* FlashScenePtr;
