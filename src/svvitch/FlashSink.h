#pragma once

#include "flash.h"
#include <Poco/format.h>
#include <Poco/Logger.h>


//------------------------------------------------------------------------
// FlashSink - Receives flash events   
//------------------------------------------------------------------------
class FlashSink: public ShockwaveFlashObjects::_IShockwaveFlashEvents {

private:
	int _ref;
	LPCONNECTIONPOINT _cp;	
	DWORD _cookie;
	//FlashPlayer*			m_pFlashPlayer;

public:
	FlashSink();

	virtual ~FlashSink();

	//----------------------------------------------------------------------------
	// Flash Control Events are handled using IDispatch. After flash control is 
	// created we retrieve a IConnectionPointContainer and try to find 
	// ShockwaveFlashObjects::_IShockwaveFlashEvents connection point which is 
	// defined with UUID "d27cdb6d-ae6d-11cf-96b8-444553540000" in this file.
	// If successful, the connection point ID is returned in m_dwCookie variable.
	// The cookie value is used to identify this connection point so we can 
	// properly shut it down later.
	//----------------------------------------------------------------------------
	HRESULT Init(ShockwaveFlashObjects::IShockwaveFlash* flash);

	//----------------------------------------------------------------------------
	// Shutdown the connection point.
	//----------------------------------------------------------------------------
	HRESULT Shutdown();
 
	//----------------------------------------------------------------------------
	// QueryInterface.
	// Overriding common COM interface method.
	//----------------------------------------------------------------------------
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppv);

	//----------------------------------------------------------------------------
	// AddRef.
	// Overriding common COM interface method.
	//----------------------------------------------------------------------------
	ULONG STDMETHODCALLTYPE AddRef();

	//----------------------------------------------------------------------------
	// Release.
	// Overriding common COM interface method.
	//----------------------------------------------------------------------------
	ULONG STDMETHODCALLTYPE Release();

	// IDispatch method
	virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* pctinfo);

	virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo);

	virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid,DISPID* rgDispId);

	virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, ::DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, ::EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr);

	HRESULT OnReadyStateChange (long newState);
    
	HRESULT OnProgress(long percentDone);

	HRESULT FSCommand(_bstr_t command, _bstr_t args);
};
