#ifdef USE_FLASH
//#include <Poco/DateTime.h>
//#include <Poco/Timezone.h>
//#include <Poco/UnicodeConverter.h>

#define _ATL_APARTMENT_THREADED
//#define _ATL_FREE_THREADED
//#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS

//#include <atlbase.h>
//#include <atlwin.h>
#include "FlashContent.h"
#include <Poco/UnicodeConverter.h>
#include "Utils.h"


FlashContent::FlashContent(Renderer& renderer): Content(renderer), _window(NULL), _flash(NULL), _cookie(0)
{
	initialize();
}

FlashContent::~FlashContent() {
	initialize();
}

void FlashContent::initialize() {
	close();
}

/** ファイルをオープンします */
bool FlashContent::open(const MediaItemPtr media, const int offset) {
	initialize();

	_window = CreateWindowEx(0, TEXT("switchClass"), 0, WS_POPUP, 0, 0, 320, 240, 0, 0, 0, 0);
	if (!_window) {
		_log.warning("failed not created flash window");
		return false;
	}
	HRESULT hr = CoCreateInstance(__uuidof(ShockwaveFlash), 0, CLSCTX_ALL, __uuidof(IShockwaveFlash), (void**)&_flash);
	if FAILED(hr) {
		_log.warning("failed not created flash object");
		return false;
	}
	//hr = _flash->QueryInterface(IID_IConnectionPointContainer, (void**)&_cpc);
	//if FAILED(hr) {
	//	_log.warning("failed not query IConnectionPointContainer");
	//	return false;
	//}
	//hr = _cpc->FindConnectionPoint(DIID__IShockwaveFlashEvents, &_cp);
	//if FAILED(hr) {
	//	_log.warning("failed not found IShockwaveFlashEvents");
	//	return false;
	//}
	_log.information("check-1");
	//hr = _cp->Advise((_IShockwaveFlashEvents*)this, &_cookie);
	//if FAILED(hr) {
	//	_log.warning("failed not advise cookie");
	//	return false;
	//}

	_log.information("check-2");
	hr = _flash->put_WMode(L"transparent");
	if FAILED(hr) {
		_log.warning("failed WMode");
		return false;
	}
	_log.information("check-3");
	//hr = AtlAxAttachControl(_flash, _window, 0);
	//if FAILED(hr) {
	//	_log.warning("failed not attached window");
	//	return false;
	//}
	//get the view object
	hr = _flash->QueryInterface(__uuidof(IViewObject),(void**)&_viewobject);
	if FAILED(hr) {
		_log.warning("failed not query viewobject");
		return false;
	}
	//create stream to Marshal view object into render thread
	//_stream = NULL;
	//hr = CoMarshalInterThreadInterfaceInStream(__uuidof(IViewObject), _viewobject, &_stream);
	//if FAILED(hr) {
	//	_log.warning("failed not create stream");
	//	return false;
	//}

	//we want it to always loop
	hr = _flash->put_Loop(true);
	_log.information("check-4");

	//load the movie
	MediaItemFile mif = media->files()[0];
	string file = Path(mif.file()).absolute(config().dataRoot).toString(Poco::Path::PATH_UNIX).substr(1);
	string sjis;
	svvitch::utf8_sjis(file, sjis);
	hr = _flash->put_Movie(_bstr_t(sjis.c_str()));
	if FAILED(hr) {
		_log.warning(Poco::format("failed put movie: %s", file));
		return false;
	}

	//since these are always going to be local media,  force wait until ready state is loaded
	//for (_state = -1; (!hr) && (_state != 4);) {
	//	hr = _flash->get_ReadyState(&_state);
	//	_log.information(Poco::format("state: %ld", _state)); // 0:読込中 1:未初期化 2:読込完了 3:制御可能 4:完了
	//	if (_state == 4) {
	//		//get the total frames of the SWF file
	//		//hr = _flash->get_TotalFrames(&cont->totalframes);
	//		break;
	//	}
	//	Sleep(0); //snooze
	//}

	set("alpha", 1.0f);
	_duration = media->duration() * 60 / 1000;
	_current = 0;
	_mediaID = media->id();
	return true;
}


/**
 * 再生
 */
void FlashContent::play() {
	_playing = true;
}

/**
 * 停止
 */
void FlashContent::stop() {
	_playing = false;
}

bool FlashContent::useFastStop() {
	return true;
}

/**
 * 再生中かどうか
 */
const bool FlashContent::playing() const {
	return _playing;
}

const bool FlashContent::finished() {
	return false;
	//return _current >= _duration;
}

/** ファイルをクローズします */
void FlashContent::close() {
	stop();
	_mediaID.clear();
	if (_cookie != 0) _cp->Unadvise(_cookie);
	SAFE_RELEASE(_flash);
}

void FlashContent::process(const DWORD& frame) {
}

void FlashContent::draw(const DWORD& frame) {
}



//implement these if we want event feedback from Flash Scripting
//DShockwaveFlashEvents
HRESULT STDMETHODCALLTYPE FlashContent::OnReadyStateChange(long newState) {
	_state = newState;
	// 0:読込中 1:未初期化 2:読込完了 3:制御可能 4:完了
	_log.information(Poco::format("flash state: %ld", _state));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashContent::OnProgress(long percentDone) {
	_log.information(Poco::format("flash progress: %ld", percentDone));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashContent::FSCommand(BSTR command, BSTR args) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashContent::FlashCall(BSTR request) {
	return S_OK;
}

//IDispatch Impl
HRESULT STDMETHODCALLTYPE FlashContent::GetTypeInfoCount(UINT __RPC_FAR *pctinfo) {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE FlashContent::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo) {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE FlashContent::GetIDsOfNames(REFIID riid, LPOLESTR __RPC_FAR *rgszNames, UINT cNames, LCID lcid, DISPID __RPC_FAR *rgDispId) {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE FlashContent::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr) {
	return S_OK;
}

//IUnknown Impl
HRESULT STDMETHODCALLTYPE FlashContent::QueryInterface(REFIID riid, void ** ppvObject) {
	if (IsEqualGUID(riid, DIID__IShockwaveFlashEvents)) {
		*ppvObject = (void*)dynamic_cast<_IShockwaveFlashEvents*>(this);
	} else if (IsEqualGUID(riid, IID_IDispatch)) {
		*ppvObject = (void*)dynamic_cast<IDispatch*>(this);
	} else if (IsEqualGUID(riid, IID_IDispatch)) {
		*ppvObject = (void*)dynamic_cast<IDispatch*>(this);
	} else if (IsEqualGUID(riid, IID_IUnknown)) {
		*ppvObject = (void*)dynamic_cast<IUnknown*>(this);
	} else {
		string serial;
		for (int i = 0; i < 8; i++) {
			serial += Poco::format(",%02?x", riid.Data4[i]);
		}
		string guid = Poco::format("[%08?x,%04?x,%04?x%s]", riid.Data1, riid.Data2, riid.Data3, serial);
		_log.warning(Poco::format("not found: %s", guid));
		*ppvObject = 0;
		return E_NOINTERFACE;
	}
	if (!(*ppvObject)) {
		_log.warning("failed no interface(dynamic_cast returned 0)");
		return E_NOINTERFACE; //if dynamic_cast returned 0
	}
	_ref++;
	return S_OK;
}

ULONG STDMETHODCALLTYPE FlashContent::AddRef() {
	_ref++;
	return _ref;
}

ULONG STDMETHODCALLTYPE FlashContent::Release() {
	_ref--;
	return _ref;
}

#endif
