#pragma once


class ControlSite : public IOleInPlaceSiteWindowless, 
					public IOleClientSite	
{
public:
	int						m_RefCount;
	FlashPlayer*			m_pFlashPlayer;

public:
	ControlSite()
	{		
		m_RefCount = 0;		
		m_pFlashPlayer = NULL;
	}	

	virtual ~ControlSite()
	{
		if (m_pFlashPlayer != NULL)
			m_pFlashPlayer->m_nCOMCount--;
	}

	void Init(FlashPlayer* pFlashPlayer)
	{
		m_pFlashPlayer = pFlashPlayer;
		m_pFlashPlayer->m_nCOMCount++;
	}

	
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppv)
	{
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


	ULONG STDMETHODCALLTYPE AddRef()
	{  
		return ++m_RefCount;
	}

	ULONG STDMETHODCALLTYPE Release()
	{ 
		int refCount = --m_RefCount;
		if (refCount == 0)		
			delete this;		

		return refCount;
	}

		//////////////////////////////////////////////////////////////////////////	

	virtual HRESULT  STDMETHODCALLTYPE SaveObject(void)
	{
		return S_OK;
	}

	virtual HRESULT  STDMETHODCALLTYPE GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker,IMoniker** ppmk )
	{
		*ppmk = NULL;
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetContainer(IOleContainer ** theContainerP)
	{
		//return QueryInterface(__uuidof(IOleContainer), (void**) theContainerP);				
		return E_NOINTERFACE;
	}


	virtual HRESULT  STDMETHODCALLTYPE ShowObject(void)
	{
		return E_NOTIMPL;
	}

		virtual HRESULT  STDMETHODCALLTYPE OnShowWindow(BOOL)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT  STDMETHODCALLTYPE RequestNewObjectLayout(void)
	{
		return E_NOTIMPL;
	}
		//		
	

	HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(/* [in] */ BOOL fEnterMode)
	{
	    return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetWindow(/* [out] */ HWND __RPC_FAR* theWnndow)
	{
		return E_FAIL;
	}

	HRESULT STDMETHODCALLTYPE CanInPlaceActivate(void)
	{
		return S_OK;
	}


	HRESULT STDMETHODCALLTYPE OnInPlaceActivate(void)
	{		
		return S_OK;
	}


	HRESULT STDMETHODCALLTYPE OnUIActivate(void)
	{		
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetWindowContext(/* [out] */ IOleInPlaceFrame __RPC_FAR *__RPC_FAR *ppFrame, /* [out] */ IOleInPlaceUIWindow __RPC_FAR *__RPC_FAR *ppDoc, /* [out] */ LPRECT lprcPosRect, /* [out] */ LPRECT lprcClipRect, /* [out][in] */ LPOLEINPLACEFRAMEINFO lpFrameInfo)
	{
		if (m_pFlashPlayer)
		{
			RECT rcRect = m_pFlashPlayer->GetRect();
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


	HRESULT STDMETHODCALLTYPE Scroll(/* [in] */ SIZE scrollExtant)
	{
		return S_OK;
	}

		HRESULT STDMETHODCALLTYPE OnUIDeactivate(/* [in] */ BOOL fUndoable)
	{		
		return S_OK;
	}


	HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate(void)
	{	
		return S_OK;
	}


	HRESULT STDMETHODCALLTYPE DiscardUndoState(void)
	{
		return S_OK;
	}


	HRESULT STDMETHODCALLTYPE DeactivateAndUndo(void)
	{
		return S_OK;
	}


	HRESULT STDMETHODCALLTYPE OnPosRectChange(/* [in] */ LPCRECT lprcPosRect)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnInPlaceActivateEx(/* [out] */ BOOL __RPC_FAR *pfNoRedraw, /* [in] */ DWORD dwFlags)
	{		
		return S_OK;
	}


	HRESULT STDMETHODCALLTYPE OnInPlaceDeactivateEx(/* [in] */ BOOL fNoRedraw)
	{		
		return S_OK;
	}


	HRESULT STDMETHODCALLTYPE RequestUIActivate(void)
	{
		return S_FALSE;
	}


	HRESULT STDMETHODCALLTYPE CanWindowlessActivate(void)
	{
		// Allow windowless activation?
		return S_OK;
	}


	HRESULT STDMETHODCALLTYPE GetCapture(void)
	{
		// TODO capture the mouse for the object
		return S_FALSE;
	}


	HRESULT STDMETHODCALLTYPE SetCapture(/* [in] */ BOOL fCapture)
	{
		// TODO capture the mouse for the object
		return S_FALSE;
	}


	HRESULT STDMETHODCALLTYPE GetFocus(void)
	{
		return S_OK;
	}


	HRESULT STDMETHODCALLTYPE SetFocus(/* [in] */ BOOL fFocus)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetDC(/* [in] */ LPCRECT pRect, /* [in] */ DWORD grfFlags, /* [out] */ HDC __RPC_FAR *phDC)
	{		
		return E_INVALIDARG;		
	}


	HRESULT STDMETHODCALLTYPE ReleaseDC(/* [in] */ HDC hDC)
	{
		return E_INVALIDARG;
	}


	HRESULT STDMETHODCALLTYPE InvalidateRect(/* [in] */ LPCRECT pRect, /* [in] */ BOOL fErase)
	{	
		if (pRect == NULL)
		{
			//m_pFlashPlayer->m_rcDirtyRect = m_pFlashPlayer->GetRect();
			//m_pFlashPlayer->m_bFlashDirty = true;
		}
		else if (!m_pFlashPlayer->m_bFlashDirty)
		{
			//SetRect(&m_pFlashPlayer->m_rcDirtyRect, pRect->left, pRect->top, pRect->right - pRect->left, pRect->bottom - pRect->top);
			//m_pFlashPlayer->m_bFlashDirty = true;
		}
		else
		{			
			
		}		
		
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE InvalidateRgn(/* [in] */ HRGN hRGN, /* [in] */ BOOL fErase)
	{	
		//m_pFlashPlayer->m_rcDirtyRect = m_pFlashPlayer->GetRect();
		//m_pFlashPlayer->m_bFlashDirty = true;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE ScrollRect(/* [in] */ INT dx, /* [in] */ INT dy, /* [in] */ LPCRECT pRectScroll, /* [in] */ LPCRECT pRectClip)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE AdjustRect(/* [out][in] */ LPRECT prc)
	{
		if (prc == NULL)
		{
			return E_INVALIDARG;
		}
		
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnDefWindowMessage(/* [in] */ UINT msg, /* [in] */ WPARAM wParam, /* [in] */ LPARAM lParam, /* [out] */ LRESULT __RPC_FAR *plResult)
	{
		return S_FALSE;
	}
};
