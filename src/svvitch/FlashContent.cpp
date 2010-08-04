#ifdef USE_FLASH
//#include <Poco/DateTime.h>
//#include <Poco/Timezone.h>
//#include <Poco/UnicodeConverter.h>


#include <atlbase.h>
#include <atlwin.h>
#include "FlashContent.h"
#include <Poco/UnicodeConverter.h>
#include "Utils.h"


FlashContent::FlashContent(Renderer& renderer): Content(renderer),
	_window(NULL), _flash(NULL), _cp(NULL), _cookie(0), _viewobject(NULL), _stream(NULL), _state(-1)
{
	initialize();
}

FlashContent::~FlashContent() {
	close();
	if (_cookie != 0 && _cp) _cp->Unadvise(_cookie);
	SAFE_RELEASE(_cp);
	SAFE_RELEASE(_flash);
	if (_window) DestroyWindow(_window);
}

void FlashContent::initialize() {
	_window = CreateWindowEx(0, TEXT("switchClass"), 0, WS_POPUP, 0, 0, 320, 240, 0, 0, 0, 0);
	if (!_window) {
		_log.warning("failed not created flash window");
		return;
	}
	HRESULT hr = CoCreateInstance(__uuidof(ShockwaveFlash), 0, CLSCTX_ALL, __uuidof(IShockwaveFlash), (void**)&_flash);
	if FAILED(hr) {
		_log.warning("failed not created flash object");
		return;
	}
	IConnectionPointContainer* cpc = NULL;
	hr = _flash->QueryInterface(IID_IConnectionPointContainer, (void**)&cpc);
	if FAILED(hr) {
		_log.warning("failed not query IConnectionPointContainer");
		return;
	}
	hr = cpc->FindConnectionPoint(DIID__IShockwaveFlashEvents, &_cp);
	if FAILED(hr) {
		_log.warning("failed not found IShockwaveFlashEvents");
		return;
	}
	SAFE_RELEASE(cpc);
	//_log.information("check-1");
	//hr = _cp->Advise((_IShockwaveFlashEvents*)this, &_cookie);
	//if FAILED(hr) {
	//	_log.warning("failed not advise cookie");
	//	return;
	//}
	//_log.information("check-2");
	hr = AtlAxAttachControl(_flash, _window, 0);
	if FAILED(hr) {
		_log.warning("failed not attached window");
		return;
	}
	//get the view object
	hr = _flash->put_WMode(L"transparent");
	if FAILED(hr) {
		_log.warning("failed WMode");
		return;
	}
	hr = _flash->QueryInterface(__uuidof(IViewObject),(void**)&_viewobject);
	if FAILED(hr) {
		_log.warning("failed not query viewobject");
		return;
	}
	//create stream to Marshal view object into render thread
	hr = CoMarshalInterThreadInterfaceInStream(__uuidof(IViewObject), _viewobject, &_stream);
	if FAILED(hr) {
		_log.warning("failed not create stream");
		return;
	}
	_log.information("flash initialized");
}

/** ファイルをオープンします */
bool FlashContent::open(const MediaItemPtr media, const int offset) {
	if (!_flash) return false;

	//HRESULT hr = _flash->DisableLocalSecurity();
	//if FAILED(hr) {
	//	_log.warning("failed DisableLocalSecurity");
	//}
	//hr = _flash->put_EmbedMovie(VARIANT_TRUE);

	//we want it to always loop
	HRESULT hr = _flash->put_Loop(VARIANT_TRUE);

	//load the movie
	_log.information(Poco::format("ready state before: %ld", _flash->ReadyState));
	MediaItemFile mif = media->files()[0];
	//string file = "file://" + Path(mif.file()).absolute(config().dataRoot).toString(Poco::Path::PATH_UNIX).substr(1);
	string file = Path(mif.file()).absolute(config().dataRoot).toString();
	string sjis;
	svvitch::utf8_sjis(file, sjis);
	hr = _flash->put_Movie(_bstr_t(sjis.c_str()));
	//wstring wfile;
	//Poco::UnicodeConverter::toUTF16(file, wfile);
	//hr = _flash->put_Movie(_bstr_t(wfile.c_str()));
	if FAILED(hr) {
		_log.warning(Poco::format("failed put movie: %s", file));
		return false;
	}
	_log.information(Poco::format("ready state after: %ld", _flash->ReadyState));
	long totalFrames = 0;
	hr = _flash->get_TotalFrames(&totalFrames);
	_log.information(Poco::format("total frames: %ld", totalFrames));
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
	if (_flash) _flash->Play();
}

/**
 * 停止
 */
void FlashContent::stop() {
	_playing = false;
	if (_flash) _flash->Stop();
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
}

void FlashContent::process(const DWORD& frame) {
	if (_flash) {
		long state = _flash->ReadyState;//-1;
		HRESULT hr;// = _flash->get_ReadyState(&state);
		if (state != _state) {
			_state = state;
			_log.information(Poco::format("state: %ld", _state)); // 0:読込中 1:未初期化 2:読込完了 3:制御可能 4:完了
			if (state == 4) {
				//get the total frames of the SWF file
				long frames;
				hr = _flash->get_TotalFrames(&frames);
				_log.information(Poco::format("total: %ld", frames));
			}
		}
	}
}

void FlashContent::draw(const DWORD& frame) {
}



//implement these if we want event feedback from Flash Scripting
//DShockwaveFlashEvents
HRESULT STDMETHODCALLTYPE FlashContent::OnReadyStateChange(long newState) {
	_state = newState;
	// 0:読込中 1:未初期化 2:読込完了 3:制御可能 4:完了
	//_log.information(Poco::format("flash state: %ld", _state));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashContent::OnProgress(long percentDone) {
	//_log.information(Poco::format("flash progress: %ld", percentDone));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashContent::FSCommand(BSTR command, BSTR args) {
	//std::wstring wc(command);
	//std::wstring wa(args);
	//string s1;
	//Poco::UnicodeConverter::toUTF8(wc, s1);
	//string s2;
	//Poco::UnicodeConverter::toUTF8(wa, s2);
	//_log.information(Poco::format("flash fscommand: %s", s1, s2));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashContent::FlashCall(BSTR request) {
	//std::wstring wreq(request);
	//string req;
	//Poco::UnicodeConverter::toUTF8(wreq, req);
	//_log.information(Poco::format("flash call: %s", req));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashContent::CreateCall(REFIID riid, IUnknown *pCtrlUnk, REFIID riid2, IUnknown **ppv) {
	string serial;
	for (int i = 0; i < 8; i++) {
		serial += Poco::format(",%02?x", riid.Data4[i]);
	}
	string guid = Poco::format("[%08?x,%04?x,%04?x%s]", riid.Data1, riid.Data2, riid.Data3, serial);
	_log.warning(Poco::format("createCall not found: %s", guid));
	return S_OK;
	//NOTIMPLEMENTED;
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
	switch (dispIdMember) {
	case 0x7a6:			
		break;
	case 0x96:			
		if ((pDispParams->cArgs == 2) &&
			(pDispParams->rgvarg[0].vt == VT_BSTR) &&
			(pDispParams->rgvarg[1].vt == VT_BSTR))
		{
			FSCommand(pDispParams->rgvarg[1].bstrVal, pDispParams->rgvarg[0].bstrVal);
		}
		break;
	case DISPID_READYSTATECHANGE:					
		break;
	default:			
		return DISP_E_MEMBERNOTFOUND;
	} 
	
	return NOERROR;
}

//IUnknown Impl
HRESULT STDMETHODCALLTYPE FlashContent::QueryInterface(REFIID riid, void ** ppvObject) {
	if (IsEqualGUID(riid, DIID__IShockwaveFlashEvents)) {
		_log.information("query _IShockwaveFlashEvents");
		*ppvObject = (void*)dynamic_cast<_IShockwaveFlashEvents*>(this);
	//} else if (IsEqualGUID(riid, IID_ICallFactory)) {
	//	*ppvObject = (void*)dynamic_cast<ICallFactory*>(this);
	//} else if (IsEqualGUID(riid, IID_IMarshal)) {
		*ppvObject = (void*)dynamic_cast<IMarshal*>(this);
	} else if (IsEqualGUID(riid, IID_IDispatch)) {
		_log.information("query IDispatch");
		*ppvObject = (void*)dynamic_cast<IDispatch*>((_IShockwaveFlashEvents*)this);
	} else if (IsEqualGUID(riid, IID_IUnknown)) {
		_log.information("query IUnknown");
		*ppvObject = (void*)dynamic_cast<IUnknown*>((_IShockwaveFlashEvents*)this);
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
