#pragma once

//#include <windows.h>
//#include <queue>
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>

#include "Content.h"
#include "flash.h"


using std::queue;
using std::string;

#define NOTIMPLEMENTED return E_NOTIMPL


class FlashContent: public Content, public _IShockwaveFlashEvents {
private:
	Poco::FastMutex _lock;

	Poco::Thread _thread;
	Poco::Runnable* _worker;

	int _ref; // 参照カウント

	HWND _window;
	IShockwaveFlash* _flash;
	IConnectionPointContainer* _cpc;
	IConnectionPoint* _cp;

	//Event Advise cookie (mmmmmmm cookies)
	DWORD _cookie;

	//IUnknown *unk;
	IViewObject* _viewobject;
	IStream* _stream;
	long _state;

	//the stream interface to marshal the viewobject into the Rendering Thread
	//IStream *pStream;

	//the RenderThread's version of the view object
	//IViewObject *RTviewobject;

	string _file;

public:
	FlashContent(Renderer& renderer);

	~FlashContent();

	// 
	void run();

	void initialize();

	/** ファイルをオープンします */
	bool open(const MediaItemPtr media, const int offset = 0);

	/**
	 * 再生
	 */
	void play();

	/**
	 * 停止
	 */
	void stop();

	bool useFastStop();

	/**
	 * 再生中かどうか
	 */
	const bool playing() const;

	const bool finished();

	/** ファイルをクローズします */
	void close();

	void process(const DWORD& frame);

	void draw(const DWORD& frame);


	//DShockwaveFlashEvents
    HRESULT STDMETHODCALLTYPE OnReadyStateChange(long newState);
    HRESULT STDMETHODCALLTYPE OnProgress(long percentDone);
    HRESULT STDMETHODCALLTYPE FSCommand(BSTR command, BSTR args);
	HRESULT STDMETHODCALLTYPE FlashCall(BSTR request);

	//IDispatch proto
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount( 
        /* [out] */ UINT __RPC_FAR *pctinfo);
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( 
        /* [in] */ UINT iTInfo,
        /* [in] */ LCID lcid,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames( 
        /* [in] */ REFIID riid,
        /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
        /* [in] */ UINT cNames,
        /* [in] */ LCID lcid,
        /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Invoke( 
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
        /* [out] */ VARIANT __RPC_FAR *pVarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
        /* [out] */ UINT __RPC_FAR *puArgErr);

	//IUnknown proto
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
};

typedef FlashContent* FlashContentPtr;