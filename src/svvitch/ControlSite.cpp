#include "ControlSite.h"

ControlSite::ControlSite(): _log(Poco::Logger::get("")), _ref(0) {
	AddRef();
}

ControlSite::~ControlSite() {
	Release();
}


HRESULT STDMETHODCALLTYPE ControlSite::QueryInterface(REFIID riid, LPVOID* ppv) {
	*ppv = NULL;

	if (riid == IID_IUnknown) {
		_log.information("query IUnknown");
		*ppv = (IUnknown*) (IOleWindow*)this;
		AddRef();
		return S_OK;
	} else if (riid == IID_IServiceProvider) {
		//_log.information("query IServiceProvider");
		return E_NOINTERFACE;

	} else if (riid == IID_ICallFactory) {
		_log.information("query ICallFactory");
		return E_NOINTERFACE;

	} else if (riid == IID_IOleWindow) {
		_log.information("query IOleWindow");
		*ppv = (IOleWindow*)this;
		AddRef();
		return S_OK;
	} else if (riid == IID_IOleInPlaceSite) {
		_log.information("query IOleInPlaceSite");
		*ppv = (IOleInPlaceSite*)this;
		AddRef();
		return S_OK;
	} else if (riid == IID_IOleInPlaceSiteEx) {
		_log.information("query IOleInPlaceSiteEx");
		*ppv = (IOleInPlaceSiteEx*)this;
		AddRef();
		return S_OK;
	} else if (riid == IID_IOleInPlaceSiteWindowless) {
		_log.information("query IOleInPlaceSiteWindowless");
		*ppv = (IOleInPlaceSiteWindowless*)this;
		AddRef();
		return S_OK;
	} else if (riid == IID_IOleClientSite) {
		_log.information("query IOleClientSite");
		*ppv = (IOleClientSite*)this;
		AddRef();
		return S_OK;
	} else if (riid == __uuidof(ShockwaveFlashObjects::_IShockwaveFlashEvents)) {
		_log.information("query _IShockwaveFlashEvents");
		*ppv = (ShockwaveFlashObjects::_IShockwaveFlashEvents*)this;
		AddRef();
		return S_OK;
	} else {
		// 1c733a30,2a1c,11ce,ad,e5,00,aa,00,44,77,3d ICallFactory
		// 6d5140c1,7436,11ce,80,34,00,aa,00,60,09,fa IServiceProvider
		std::string serial;
		for (int i = 0; i < 8; i++) {
			serial += Poco::format(",%02?x", riid.Data4[i]);
		}
		std::string guid = Poco::format("[%08?x,%04?x,%04?x%s]", riid.Data1, riid.Data2, riid.Data3, serial);
		_log.information(Poco::format("query REFIID: %s", guid));

		return E_NOINTERFACE; // E_NOTIMPL
	}
}

ULONG STDMETHODCALLTYPE ControlSite::AddRef() {
	return ++_ref;
}

ULONG STDMETHODCALLTYPE ControlSite::Release() {
	int ref = --_ref;
	if (ref == 0) delete this;
	return ref;
}


HRESULT STDMETHODCALLTYPE ControlSite::SaveObject(void) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker,IMoniker** ppmk) {
	*ppmk = NULL;
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE ControlSite::GetContainer(IOleContainer** theContainerP) {
	return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE ControlSite::ShowObject(void) {
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE ControlSite::OnShowWindow(BOOL) {
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE ControlSite::RequestNewObjectLayout(void) {
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE ControlSite::GetWindow(HWND __RPC_FAR* theWnndow) {
	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE ControlSite::ContextSensitiveHelp(BOOL fEnterMode) {
    return S_OK;
}


HRESULT STDMETHODCALLTYPE ControlSite::CanInPlaceActivate(void) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::OnInPlaceActivate(void) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::OnUIActivate(void) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::GetWindowContext(IOleInPlaceFrame __RPC_FAR *__RPC_FAR *ppFrame, IOleInPlaceUIWindow __RPC_FAR *__RPC_FAR *ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo) {
	_log.information("GetWindowContext");
	//if (m_pFlashPlayer)
	{
		RECT rcRect = {0};
		//RECT rcRect = m_pFlashPlayer->GetRect();

		*lprcPosRect = rcRect;
		*lprcClipRect = rcRect;
		
		*ppFrame = NULL;
		QueryInterface(__uuidof(IOleInPlaceFrame), (void**)ppFrame);		
		*ppDoc = NULL;

		lpFrameInfo->fMDIApp = FALSE;
		lpFrameInfo->hwndFrame = NULL;
		lpFrameInfo->haccel = NULL;
		lpFrameInfo->cAccelEntries = 0;

		return S_OK;
	}

	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE ControlSite::Scroll(SIZE scrollExtant) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::OnUIDeactivate(BOOL fUndoable) {
	_log.information("OnUIDeactivate");
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::OnInPlaceDeactivate(void) {
	_log.information("OnInPlaceDeactivate");
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::DiscardUndoState(void) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::DeactivateAndUndo(void) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::OnPosRectChange(LPCRECT lprcPosRect) {
	return S_OK;
}


HRESULT STDMETHODCALLTYPE ControlSite::OnInPlaceActivateEx(BOOL __RPC_FAR *pfNoRedraw, DWORD dwFlags) {
	_log.information("OnInPlaceActivateEx");
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::OnInPlaceDeactivateEx(BOOL fNoRedraw) {
	_log.information("OnInPlaceDeactivateEx");
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::RequestUIActivate(void) {
	_log.information("RequestUIActivate");
	return S_FALSE;
}


HRESULT STDMETHODCALLTYPE ControlSite::CanWindowlessActivate(void) {
	// Allow windowless activation?
	_log.information("CanWindowlessActivate");
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::GetCapture(void) {
	// TODO capture the mouse for the object
	_log.information("GetCapture");
	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE ControlSite::SetCapture(BOOL fCapture) {
	// TODO capture the mouse for the object
	_log.information("SetCapture");
	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE ControlSite::GetFocus(void) {
	_log.information("GetFocus");
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::SetFocus(BOOL fFocus) {
	_log.information("SetFocus");
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::GetDC(LPCRECT pRect, DWORD grfFlags, HDC __RPC_FAR *phDC) {
	_log.information("GetDC");
	return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE ControlSite::ReleaseDC(HDC hDC) {
	_log.information("ReleaseDC");
	return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE ControlSite::InvalidateRect(LPCRECT pRect, BOOL fErase) {	
	_log.information("InvalidateRect");
	if (pRect == NULL) {
		//m_pFlashPlayer->m_rcDirtyRect = m_pFlashPlayer->GetRect();
		//m_pFlashPlayer->m_bFlashDirty = true;
	//} else if (!m_pFlashPlayer->m_bFlashDirty) {
		//SetRect(&m_pFlashPlayer->m_rcDirtyRect, pRect->left, pRect->top, pRect->right - pRect->left, pRect->bottom - pRect->top);
		//m_pFlashPlayer->m_bFlashDirty = true;
	} else {			
		
	}		

	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::InvalidateRgn(HRGN hRGN, BOOL fErase) {	
	_log.information("InvalidateRgn");
	//m_pFlashPlayer->m_rcDirtyRect = m_pFlashPlayer->GetRect();
	//m_pFlashPlayer->m_bFlashDirty = true;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::ScrollRect(INT dx, INT dy, LPCRECT pRectScroll, LPCRECT pRectClip) {
	_log.information("ScrollRect");
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::AdjustRect(LPRECT prc) {
	_log.information("AdjustRect");
	if (prc == NULL) {
		return E_INVALIDARG;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::OnDefWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT __RPC_FAR *plResult) {
	_log.information("OnDefWindowMessage");
	return S_FALSE;
}
