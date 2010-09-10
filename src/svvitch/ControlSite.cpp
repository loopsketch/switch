#include "ControlSite.h"
#include "FlashContent.h"

#include <Poco/format.h>

using std::string;


ControlSite::ControlSite(FlashContentPtr flash): _log(Poco::Logger::get("")), _flash(flash)
{
	_ref = 0;
	::SetRect(&_rect, 0, 0, 2000, 2000);
}	

ControlSite::~ControlSite() {
}

void ControlSite::GetRect(LPRECT rect) {
	SetRect(rect, _rect.left, _rect.top, _rect.right, _rect.bottom);
}

HRESULT STDMETHODCALLTYPE ControlSite::QueryInterface(REFIID riid, LPVOID* ppv) {
	*ppv = NULL;

	if (riid == IID_IUnknown)
	{
		*ppv = (IUnknown*) (IOleWindow*) this;
		AddRef();
		return S_OK;
	}
	else if (riid == IID_IOleWindow)
	{
		*ppv = (IOleWindow*)this;
		AddRef();
		return S_OK;
	}
	else if (riid == IID_IOleInPlaceSite)
	{
		*ppv = (IOleInPlaceSite*)this;
		AddRef();
		return S_OK;
	}
	else if (riid == IID_IOleInPlaceSiteEx)
	{
		*ppv = (IOleInPlaceSiteEx*)this;
		AddRef();
		return S_OK;
	}
	else if (riid == IID_IOleInPlaceSiteWindowless)
	{
		*ppv = (IOleInPlaceSiteWindowless*)this;
		AddRef();
		return S_OK;
	}
	else if (riid == IID_IOleClientSite)
	{
		*ppv = (IOleClientSite*)this;
		AddRef();
		return S_OK;
	}
	else if (riid == __uuidof(ShockwaveFlashObjects::_IShockwaveFlashEvents))
	{
		*ppv = (ShockwaveFlashObjects::_IShockwaveFlashEvents*) this;
		AddRef();
		return S_OK;
	}
	else
	{   
		return E_NOTIMPL;
	}
}


ULONG STDMETHODCALLTYPE ControlSite::AddRef()
{  
	return ++_ref;
}

ULONG STDMETHODCALLTYPE ControlSite::Release()
{ 
	int ref = --_ref;
	if (ref == 0)		
		delete this;		

	return ref;
}

	//////////////////////////////////////////////////////////////////////////	

HRESULT  STDMETHODCALLTYPE ControlSite::SaveObject(void) {
	return S_OK;
}

HRESULT  STDMETHODCALLTYPE ControlSite::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker,IMoniker** ppmk ) {
	*ppmk = NULL;
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE ControlSite::GetContainer(IOleContainer ** theContainerP) {
	//return QueryInterface(__uuidof(IOleContainer), (void**) theContainerP);				
	return E_NOINTERFACE;
}


HRESULT STDMETHODCALLTYPE ControlSite::ShowObject(void) {
	return E_NOTIMPL;
}

HRESULT  STDMETHODCALLTYPE ControlSite::OnShowWindow(BOOL) {
	return E_NOTIMPL;
}

HRESULT  STDMETHODCALLTYPE ControlSite::RequestNewObjectLayout(void) {
	return E_NOTIMPL;
}
	//		


HRESULT STDMETHODCALLTYPE ControlSite::ContextSensitiveHelp(/* [in] */ BOOL fEnterMode) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::GetWindow(/* [out] */ HWND __RPC_FAR* theWnndow) {
	return E_FAIL;
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

HRESULT STDMETHODCALLTYPE ControlSite::GetWindowContext(/* [out] */ IOleInPlaceFrame __RPC_FAR *__RPC_FAR *ppFrame, /* [out] */ IOleInPlaceUIWindow __RPC_FAR *__RPC_FAR *ppDoc, /* [out] */ LPRECT lprcPosRect, /* [out] */ LPRECT lprcClipRect, /* [out][in] */ LPOLEINPLACEFRAMEINFO lpFrameInfo) {
	//if (m_pFlashPlayer)
	{
		//RECT rcRect = m_pFlashPlayer->GetRect();
		RECT rcRect;
		//RECT rcRect = m_pFlashPlayer->m_rcDirtyRect;   

		*lprcPosRect = rcRect;
		*lprcClipRect = rcRect;
		
		*ppFrame = NULL;
		QueryInterface(__uuidof(IOleInPlaceFrame), (void**) ppFrame);		
		*ppDoc = NULL;

		lpFrameInfo->fMDIApp = FALSE;
		lpFrameInfo->hwndFrame = NULL;
		lpFrameInfo->haccel = NULL;
		lpFrameInfo->cAccelEntries = 0;

		return S_OK;
	}

	return E_FAIL;
}


HRESULT STDMETHODCALLTYPE ControlSite::Scroll(/* [in] */ SIZE scrollExtant) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::OnUIDeactivate(/* [in] */ BOOL fUndoable) {		
	return S_OK;
}


HRESULT STDMETHODCALLTYPE ControlSite::OnInPlaceDeactivate(void) {	
	return S_OK;
}


HRESULT STDMETHODCALLTYPE ControlSite::DiscardUndoState(void) {
	return S_OK;
}


HRESULT STDMETHODCALLTYPE ControlSite::DeactivateAndUndo(void) {
	return S_OK;
}


HRESULT STDMETHODCALLTYPE ControlSite::OnPosRectChange(/* [in] */ LPCRECT lprcPosRect) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::OnInPlaceActivateEx(/* [out] */ BOOL __RPC_FAR *pfNoRedraw, /* [in] */ DWORD dwFlags) {		
	return S_OK;
}


HRESULT STDMETHODCALLTYPE ControlSite::OnInPlaceDeactivateEx(/* [in] */ BOOL fNoRedraw) {
	return S_OK;
}


HRESULT STDMETHODCALLTYPE ControlSite::RequestUIActivate(void) {
	return S_FALSE;
}


HRESULT STDMETHODCALLTYPE ControlSite::CanWindowlessActivate(void) {
	// Allow windowless activation?
	return S_OK;
}


HRESULT STDMETHODCALLTYPE ControlSite::GetCapture(void) {
	// TODO capture the mouse for the object
	return S_FALSE;
}


HRESULT STDMETHODCALLTYPE ControlSite::SetCapture(/* [in] */ BOOL fCapture) {
	// TODO capture the mouse for the object
	return S_FALSE;
}


HRESULT STDMETHODCALLTYPE ControlSite::GetFocus(void) {
	return S_OK;
}


HRESULT STDMETHODCALLTYPE ControlSite::SetFocus(/* [in] */ BOOL fFocus) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::GetDC(/* [in] */ LPCRECT pRect, /* [in] */ DWORD grfFlags, /* [out] */ HDC __RPC_FAR *phDC) {
	return E_INVALIDARG;		
}


HRESULT STDMETHODCALLTYPE ControlSite::ReleaseDC(/* [in] */ HDC hDC) {
	return E_INVALIDARG;
}


HRESULT STDMETHODCALLTYPE ControlSite::InvalidateRect(/* [in] */ LPCRECT rect, /* [in] */ BOOL erase) {
	if (rect) {
		SetRect(&_rect, rect->left, rect->top, rect->right, rect->bottom);
		_flash->update();
	}
	
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::InvalidateRgn(/* [in] */ HRGN hRGN, /* [in] */ BOOL fErase) {
	//m_pFlashPlayer->m_rcDirtyRect = m_pFlashPlayer->GetRect();
	//m_pFlashPlayer->m_bFlashDirty = true;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::ScrollRect(/* [in] */ INT dx, /* [in] */ INT dy, /* [in] */ LPCRECT pRectScroll, /* [in] */ LPCRECT pRectClip) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::AdjustRect(/* [out][in] */ LPRECT prc) {
	if (prc == NULL)
	{
		return E_INVALIDARG;
	}
	
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ControlSite::OnDefWindowMessage(/* [in] */ UINT msg, /* [in] */ WPARAM wParam, /* [in] */ LPARAM lParam, /* [out] */ LRESULT __RPC_FAR *plResult) {
	return S_FALSE;
}
