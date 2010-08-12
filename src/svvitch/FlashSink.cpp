#include "FlashSink.h"
#include "Common.h"

FlashSink::FlashSink() {
}

FlashSink::~FlashSink() {
}

HRESULT FlashSink::Init(ShockwaveFlashObjects::IShockwaveFlash* flash) {
	//m_pFlashPlayer = pFlashPlayer;
	//m_pFlashPlayer->m_nCOMCount++;

	HRESULT hr = NOERROR;
	LPCONNECTIONPOINTCONTAINER cpc = NULL;
	if ((flash->QueryInterface(IID_IConnectionPointContainer, (void**)&cpc) == S_OK) &&
		(cpc->FindConnectionPoint(__uuidof(ShockwaveFlashObjects::_IShockwaveFlashEvents), &_cp) == S_OK))			
	{
		IDispatch* dispatch = NULL;
		QueryInterface(__uuidof(IDispatch), (void**)&dispatch);
		if (dispatch != NULL) {
			hr = _cp->Advise((LPUNKNOWN)dispatch, &_cookie);
			dispatch->Release();
		}
	}
	if (cpc) cpc->Release();
	return hr;
}

HRESULT FlashSink::Shutdown() {
	HRESULT hr = S_OK;
	if (_cp) {
		if (_cookie) {
			hr = _cp->Unadvise(_cookie);
			_cookie = 0;
		}
		SAFE_RELEASE(_cp);
	}
	return hr;
}


HRESULT STDMETHODCALLTYPE FlashSink::QueryInterface(REFIID riid, LPVOID* ppv) {
	*ppv = NULL;

	if (riid == IID_IUnknown) {
		*ppv = (LPUNKNOWN)this;
		AddRef();
		return S_OK;
	} else if (riid == IID_IDispatch) {
		*ppv = (IDispatch*)this;
		AddRef();
		return S_OK;
	} else if (riid == __uuidof(ShockwaveFlashObjects::_IShockwaveFlashEvents)) {
		*ppv = (ShockwaveFlashObjects::_IShockwaveFlashEvents*)this;
		AddRef();
		return S_OK;
	} else {   
		return E_NOTIMPL;
	}
}

ULONG STDMETHODCALLTYPE FlashSink::AddRef() {
	return ++_ref;
}

ULONG STDMETHODCALLTYPE FlashSink::Release() {
	int ref = --_ref;
	if (ref == 0) delete this;
	return ref;
}


HRESULT STDMETHODCALLTYPE FlashSink::GetTypeInfoCount(UINT* pctinfo) {
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE FlashSink::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) {
	return E_NOTIMPL; 
}

HRESULT STDMETHODCALLTYPE FlashSink::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid,DISPID* rgDispId) {
	return E_NOTIMPL; 
}

HRESULT STDMETHODCALLTYPE FlashSink::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, ::DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, ::EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr) {
	switch(dispIdMember) {
	case 0x7a6:			
		break;
	case 0x96:			
		if ((pDispParams->cArgs == 2) && (pDispParams->rgvarg[0].vt == VT_BSTR) && (pDispParams->rgvarg[1].vt == VT_BSTR))
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

HRESULT FlashSink::OnReadyStateChange(long newState) {	
	return S_OK;
}

HRESULT FlashSink::OnProgress(long percentDone) {		
	return S_OK;
}

HRESULT FlashSink::FSCommand(_bstr_t command, _bstr_t args) {
	//if (m_pFlashPlayer->m_pFlashListener != NULL)
	//	m_pFlashPlayer->m_pFlashListener->FlashCommand((char*) command, (char*) args);
	return S_OK;
}
