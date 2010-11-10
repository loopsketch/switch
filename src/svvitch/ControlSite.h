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
#include <atlbase.h>
#include <Poco/Logger.h>
#include "ComContent.h"


class ControlSite: public IOleInPlaceSiteWindowless, public IOleClientSite
{
private:
	Poco::Logger& _log;
	int _ref;
	RECT _rect;
	ComContentPtr _com;

public:
	ControlSite(ComContentPtr com);

	virtual ~ControlSite();

	void GetRect(LPRECT rect);

	// IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppv);

	ULONG STDMETHODCALLTYPE AddRef();

	ULONG STDMETHODCALLTYPE Release();



	// IServiceProvider
    //virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject);



	// ICallFactory
	//HRESULT STDMETHODCALLTYPE CreateCall(REFIID riid, IUnknown *pCtrlUnk, REFIID riid2, IUnknown **ppv);



	// IOleClientSite

	virtual HRESULT STDMETHODCALLTYPE SaveObject(void);

	virtual HRESULT STDMETHODCALLTYPE GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker,IMoniker** ppmk);

	virtual HRESULT STDMETHODCALLTYPE GetContainer(IOleContainer ** theContainerP);

	virtual HRESULT STDMETHODCALLTYPE ShowObject(void);

	virtual HRESULT STDMETHODCALLTYPE OnShowWindow(BOOL);

	virtual HRESULT STDMETHODCALLTYPE RequestNewObjectLayout(void);



	// IOleWindow

	HRESULT STDMETHODCALLTYPE GetWindow(HWND __RPC_FAR* theWnndow);

	HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);



	// IOleInPlaceSite

	HRESULT STDMETHODCALLTYPE CanInPlaceActivate(void);

	HRESULT STDMETHODCALLTYPE OnInPlaceActivate(void);

	HRESULT STDMETHODCALLTYPE OnUIActivate(void);

	HRESULT STDMETHODCALLTYPE GetWindowContext(IOleInPlaceFrame __RPC_FAR *__RPC_FAR *ppFrame, IOleInPlaceUIWindow __RPC_FAR *__RPC_FAR *ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo);

	HRESULT STDMETHODCALLTYPE Scroll(SIZE scrollExtant);

	HRESULT STDMETHODCALLTYPE OnUIDeactivate(BOOL fUndoable);

	HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate(void);

	HRESULT STDMETHODCALLTYPE DiscardUndoState(void);

	HRESULT STDMETHODCALLTYPE DeactivateAndUndo(void);

	HRESULT STDMETHODCALLTYPE OnPosRectChange(LPCRECT lprcPosRect);



	// IOleInPlaceSiteEx

	HRESULT STDMETHODCALLTYPE OnInPlaceActivateEx(BOOL __RPC_FAR *pfNoRedraw, DWORD dwFlags);

	HRESULT STDMETHODCALLTYPE OnInPlaceDeactivateEx(BOOL fNoRedraw);

	HRESULT STDMETHODCALLTYPE RequestUIActivate(void);



	// IOleInPlaceSiteWindowless

	HRESULT STDMETHODCALLTYPE CanWindowlessActivate(void);

	HRESULT STDMETHODCALLTYPE GetCapture(void);

	HRESULT STDMETHODCALLTYPE SetCapture(BOOL fCapture);

	HRESULT STDMETHODCALLTYPE GetFocus(void);

	HRESULT STDMETHODCALLTYPE SetFocus(BOOL fFocus);

	HRESULT STDMETHODCALLTYPE GetDC(LPCRECT pRect, DWORD grfFlags, HDC __RPC_FAR *phDC);

	HRESULT STDMETHODCALLTYPE ReleaseDC(HDC hDC);

	HRESULT STDMETHODCALLTYPE InvalidateRect(LPCRECT pRect, BOOL fErase);

	HRESULT STDMETHODCALLTYPE InvalidateRgn(HRGN hRGN, BOOL fErase);

	HRESULT STDMETHODCALLTYPE ScrollRect(INT dx, INT dy, LPCRECT pRectScroll, LPCRECT pRectClip);

	HRESULT STDMETHODCALLTYPE AdjustRect(LPRECT prc);

	HRESULT STDMETHODCALLTYPE OnDefWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT __RPC_FAR *plResult);
};
