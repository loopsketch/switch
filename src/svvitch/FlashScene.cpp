#ifdef USE_FLASH

//#define _ATL_APARTMENT_THREADED
//#define _ATL_FREE_THREADED
//#define _ATL_NO_AUTOMATIC_NAMESPACE
//#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS

#include "FlashScene.h"


FlashScene::FlashScene(Renderer& renderer): Scene(renderer),
	_controlSite(NULL), _ole(NULL), _flash(NULL), _windowless(NULL), _view(NULL)
{
}

FlashScene::~FlashScene() {
}

bool FlashScene::initialize() {
	_controlSite = new ControlSite();
	//_controlSite->Init(this);

	// Œ³XCLSCTX_ALL CLSCTX_INPROC_SERVER CLSID_ShockwaveFlash
	HRESULT hr = CoCreateInstance(__uuidof(ShockwaveFlash), 0, CLSCTX_ALL, IID_IOleObject, (void**)&_ole);
	if FAILED(hr) {
		_log.warning("failed not created 'ShockwaveFlash' class");
		return false;
	}

	IOleClientSite* clientSite = NULL;
	hr = _controlSite->QueryInterface(__uuidof(IOleClientSite), (void**)&clientSite);
	if FAILED(hr) {
		_log.warning("failed not query 'IOleClientSite'");
		return false;
	}
	hr = _ole->SetClientSite(clientSite);

	hr = _ole->QueryInterface(__uuidof(IShockwaveFlash), (LPVOID*)&_flash);
	if FAILED(hr) {
		_log.warning("failed not query 'IShockwaveFlash'");
		return false;
	}
	_bstr_t szTrans = "Transparent";
	hr = _flash->put_WMode(szTrans);

	hr = _ole->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, clientSite, 0, NULL, NULL);
	if FAILED(hr) {
		_log.warning("failed DoVerb");
		//return;
	}
	clientSite->Release();

	hr = _ole->QueryInterface(__uuidof(IOleInPlaceObjectWindowless), (LPVOID*)&_windowless);
	if FAILED(hr) {
		_log.warning("failed not query 'IOleInPlaceObjectWindowless'");
		//return;
	}
	hr = _flash->QueryInterface(IID_IViewObject, (LPVOID*)&_view);
	if FAILED(hr) {
		_log.warning("failed not query 'IViewObject'");
		return false;
	}

	_log.information("flash initialized(thread)");
	return true;
}

void FlashScene::process() {
}

void FlashScene::draw1() {
}

void FlashScene::draw2() {
}

#endif