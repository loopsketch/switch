#pragma once


class ControlSite: public IOleInPlaceSiteWindowless, public IOleClientSite	
{
public:
	int						m_RefCount;
	FlashPlayer*			m_pFlashPlayer;

public:
	ControlSite();

	virtual ~ControlSite();

	void Init(FlashPlayer* pFlashPlayer);

	
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppv);

	ULONG STDMETHODCALLTYPE AddRef();

	ULONG STDMETHODCALLTYPE Release();

		//////////////////////////////////////////////////////////////////////////	

	virtual HRESULT  STDMETHODCALLTYPE SaveObject(void);

	virtual HRESULT  STDMETHODCALLTYPE GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker,IMoniker** ppmk );

	virtual HRESULT STDMETHODCALLTYPE GetContainer(IOleContainer ** theContainerP);


	virtual HRESULT  STDMETHODCALLTYPE ShowObject(void);

	virtual HRESULT  STDMETHODCALLTYPE OnShowWindow(BOOL);

	virtual HRESULT  STDMETHODCALLTYPE RequestNewObjectLayout(void);
		//		
	

	HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(/* [in] */ BOOL fEnterMode);

	HRESULT STDMETHODCALLTYPE GetWindow(/* [out] */ HWND __RPC_FAR* theWnndow);

	HRESULT STDMETHODCALLTYPE CanInPlaceActivate(void);


	HRESULT STDMETHODCALLTYPE OnInPlaceActivate(void);


	HRESULT STDMETHODCALLTYPE OnUIActivate(void);

	HRESULT STDMETHODCALLTYPE GetWindowContext(/* [out] */ IOleInPlaceFrame __RPC_FAR *__RPC_FAR *ppFrame, /* [out] */ IOleInPlaceUIWindow __RPC_FAR *__RPC_FAR *ppDoc, /* [out] */ LPRECT lprcPosRect, /* [out] */ LPRECT lprcClipRect, /* [out][in] */ LPOLEINPLACEFRAMEINFO lpFrameInfo);


	HRESULT STDMETHODCALLTYPE Scroll(/* [in] */ SIZE scrollExtant);

	HRESULT STDMETHODCALLTYPE OnUIDeactivate(/* [in] */ BOOL fUndoable);


	HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate(void);


	HRESULT STDMETHODCALLTYPE DiscardUndoState(void);


	HRESULT STDMETHODCALLTYPE DeactivateAndUndo(void);


	HRESULT STDMETHODCALLTYPE OnPosRectChange(/* [in] */ LPCRECT lprcPosRect);

	HRESULT STDMETHODCALLTYPE OnInPlaceActivateEx(/* [out] */ BOOL __RPC_FAR *pfNoRedraw, /* [in] */ DWORD dwFlags);


	HRESULT STDMETHODCALLTYPE OnInPlaceDeactivateEx(/* [in] */ BOOL fNoRedraw);


	HRESULT STDMETHODCALLTYPE RequestUIActivate(void);


	HRESULT STDMETHODCALLTYPE CanWindowlessActivate(void);


	HRESULT STDMETHODCALLTYPE GetCapture(void);


	HRESULT STDMETHODCALLTYPE SetCapture(/* [in] */ BOOL fCapture);


	HRESULT STDMETHODCALLTYPE GetFocus(void);


	HRESULT STDMETHODCALLTYPE SetFocus(/* [in] */ BOOL fFocus);

	HRESULT STDMETHODCALLTYPE GetDC(/* [in] */ LPCRECT pRect, /* [in] */ DWORD grfFlags, /* [out] */ HDC __RPC_FAR *phDC);


	HRESULT STDMETHODCALLTYPE ReleaseDC(/* [in] */ HDC hDC);


	HRESULT STDMETHODCALLTYPE InvalidateRect(/* [in] */ LPCRECT pRect, /* [in] */ BOOL fErase);

	HRESULT STDMETHODCALLTYPE InvalidateRgn(/* [in] */ HRGN hRGN, /* [in] */ BOOL fErase)
	{	
		//m_pFlashPlayer->m_rcDirtyRect = m_pFlashPlayer->GetRect();
		//m_pFlashPlayer->m_bFlashDirty = true;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE ScrollRect(/* [in] */ INT dx, /* [in] */ INT dy, /* [in] */ LPCRECT pRectScroll, /* [in] */ LPCRECT pRectClip);

	HRESULT STDMETHODCALLTYPE AdjustRect(/* [out][in] */ LPRECT prc);

	HRESULT STDMETHODCALLTYPE OnDefWindowMessage(/* [in] */ UINT msg, /* [in] */ WPARAM wParam, /* [in] */ LPARAM lParam, /* [out] */ LRESULT __RPC_FAR *plResult);
};
