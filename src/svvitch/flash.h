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

#include <windows.h>
#include <atlbase.h>

#pragma warning(disable: 4192)
// Created by Microsoft (R) C/C++ Compiler Version 15.00.30729.01 (e0ae33f4).
//
// e:\vs_workspace\switch_sf\release\flash10i.tlh
//
// C++ source equivalent of Win32 type library C:\\WINDOWS\\system32\\macromed\\Flash\\Flash10i.ocx
// compiler-generated file created 08/17/10 at 17:15:23 - DO NOT EDIT!

#pragma once
#pragma pack(push, 8)

#include <comdef.h>

namespace ShockwaveFlashObjects {

//
// Forward references and typedefs
//

struct __declspec(uuid("d27cdb6b-ae6d-11cf-96b8-444553540000"))
/* LIBID */ __ShockwaveFlashObjects;
struct __declspec(uuid("d27cdb6c-ae6d-11cf-96b8-444553540000"))
/* dual interface */ IShockwaveFlash;
struct __declspec(uuid("c5598e60-b307-11d1-b27d-006008c3fbfb"))
/* interface */ ICanHandleException;
struct __declspec(uuid("d27cdb6d-ae6d-11cf-96b8-444553540000"))
/* dispinterface */ _IShockwaveFlashEvents;
struct /* coclass */ ShockwaveFlash;
struct __declspec(uuid("d27cdb70-ae6d-11cf-96b8-444553540000"))
/* interface */ IFlashFactory;
struct __declspec(uuid("d27cdb72-ae6d-11cf-96b8-444553540000"))
/* interface */ IFlashObjectInterface;
struct __declspec(uuid("a6ef9860-c720-11d0-9337-00a0c90dcaa9"))
/* interface */ IDispatchEx;
struct /* coclass */ FlashObjectInterface;
struct __declspec(uuid("86230738-d762-4c50-a2de-a753e5b1686f"))
/* dual interface */ IFlashObject;
struct /* coclass */ FlashObject;

//
// Smart pointer typedef declarations
//

_COM_SMARTPTR_TYPEDEF(IShockwaveFlash, __uuidof(IShockwaveFlash));
_COM_SMARTPTR_TYPEDEF(ICanHandleException, __uuidof(ICanHandleException));
_COM_SMARTPTR_TYPEDEF(_IShockwaveFlashEvents, __uuidof(_IShockwaveFlashEvents));
_COM_SMARTPTR_TYPEDEF(IFlashFactory, __uuidof(IFlashFactory));
_COM_SMARTPTR_TYPEDEF(IDispatchEx, __uuidof(IDispatchEx));
_COM_SMARTPTR_TYPEDEF(IFlashObjectInterface, __uuidof(IFlashObjectInterface));
_COM_SMARTPTR_TYPEDEF(IFlashObject, __uuidof(IFlashObject));

//
// Type library items
//

struct __declspec(uuid("d27cdb6c-ae6d-11cf-96b8-444553540000"))
IShockwaveFlash : IDispatch
{
    //
    // Property data
    //

    __declspec(property(get=GetScaleMode,put=PutScaleMode))
    int ScaleMode;
    __declspec(property(get=GetAlignMode,put=PutAlignMode))
    int AlignMode;
    __declspec(property(get=GetBackgroundColor,put=PutBackgroundColor))
    long BackgroundColor;
    __declspec(property(get=GetTotalFrames))
    long TotalFrames;
    __declspec(property(get=GetPlaying,put=PutPlaying))
    VARIANT_BOOL Playing;
    __declspec(property(get=GetMovieData,put=PutMovieData))
    _bstr_t MovieData;
    __declspec(property(get=GetInlineData,put=PutInlineData))
    IUnknownPtr InlineData;
    __declspec(property(get=GetSeamlessTabbing,put=PutSeamlessTabbing))
    VARIANT_BOOL SeamlessTabbing;
    __declspec(property(get=GetWMode,put=PutWMode))
    _bstr_t WMode;
    __declspec(property(get=GetSAlign,put=PutSAlign))
    _bstr_t SAlign;
    __declspec(property(get=GetMenu,put=PutMenu))
    VARIANT_BOOL Menu;
    __declspec(property(get=GetBase,put=PutBase))
    _bstr_t Base;
    __declspec(property(get=GetScale,put=PutScale))
    _bstr_t Scale;
    __declspec(property(get=GetDeviceFont,put=PutDeviceFont))
    VARIANT_BOOL DeviceFont;
    __declspec(property(get=GetEmbedMovie,put=PutEmbedMovie))
    VARIANT_BOOL EmbedMovie;
    __declspec(property(get=GetBGColor,put=PutBGColor))
    _bstr_t BGColor;
    __declspec(property(get=GetQuality2,put=PutQuality2))
    _bstr_t Quality2;
    __declspec(property(get=GetProfile,put=PutProfile))
    VARIANT_BOOL Profile;
    __declspec(property(get=GetProfileAddress,put=PutProfileAddress))
    _bstr_t ProfileAddress;
    __declspec(property(get=GetProfilePort,put=PutProfilePort))
    long ProfilePort;
    __declspec(property(get=GetAllowNetworking,put=PutAllowNetworking))
    _bstr_t AllowNetworking;
    __declspec(property(get=GetAllowFullScreen,put=PutAllowFullScreen))
    _bstr_t AllowFullScreen;
    __declspec(property(get=GetReadyState))
    long ReadyState;
    __declspec(property(get=GetSWRemote,put=PutSWRemote))
    _bstr_t SWRemote;
    __declspec(property(get=GetMovie,put=PutMovie))
    _bstr_t Movie;
    __declspec(property(get=GetQuality,put=PutQuality))
    int Quality;
    __declspec(property(get=GetLoop,put=PutLoop))
    VARIANT_BOOL Loop;
    __declspec(property(get=GetFrameNum,put=PutFrameNum))
    long FrameNum;
    __declspec(property(get=GetFlashVars,put=PutFlashVars))
    _bstr_t FlashVars;
    __declspec(property(get=GetAllowScriptAccess,put=PutAllowScriptAccess))
    _bstr_t AllowScriptAccess;

    //
    // Wrapper methods for error-handling
    //

    long GetReadyState ( );
    long GetTotalFrames ( );
    VARIANT_BOOL GetPlaying ( );
    void PutPlaying (
        VARIANT_BOOL pVal );
    int GetQuality ( );
    void PutQuality (
        int pVal );
    int GetScaleMode ( );
    void PutScaleMode (
        int pVal );
    int GetAlignMode ( );
    void PutAlignMode (
        int pVal );
    long GetBackgroundColor ( );
    void PutBackgroundColor (
        long pVal );
    VARIANT_BOOL GetLoop ( );
    void PutLoop (
        VARIANT_BOOL pVal );
    _bstr_t GetMovie ( );
    void PutMovie (
        _bstr_t pVal );
    long GetFrameNum ( );
    void PutFrameNum (
        long pVal );
    HRESULT SetZoomRect (
        long left,
        long top,
        long right,
        long bottom );
    HRESULT Zoom (
        int factor );
    HRESULT Pan (
        long x,
        long y,
        int mode );
    HRESULT Play ( );
    HRESULT Stop ( );
    HRESULT Back ( );
    HRESULT Forward ( );
    HRESULT Rewind ( );
    HRESULT StopPlay ( );
    HRESULT GotoFrame (
        long FrameNum );
    long CurrentFrame ( );
    VARIANT_BOOL IsPlaying ( );
    long PercentLoaded ( );
    VARIANT_BOOL FrameLoaded (
        long FrameNum );
    long FlashVersion ( );
    _bstr_t GetWMode ( );
    void PutWMode (
        _bstr_t pVal );
    _bstr_t GetSAlign ( );
    void PutSAlign (
        _bstr_t pVal );
    VARIANT_BOOL GetMenu ( );
    void PutMenu (
        VARIANT_BOOL pVal );
    _bstr_t GetBase ( );
    void PutBase (
        _bstr_t pVal );
    _bstr_t GetScale ( );
    void PutScale (
        _bstr_t pVal );
    VARIANT_BOOL GetDeviceFont ( );
    void PutDeviceFont (
        VARIANT_BOOL pVal );
    VARIANT_BOOL GetEmbedMovie ( );
    void PutEmbedMovie (
        VARIANT_BOOL pVal );
    _bstr_t GetBGColor ( );
    void PutBGColor (
        _bstr_t pVal );
    _bstr_t GetQuality2 ( );
    void PutQuality2 (
        _bstr_t pVal );
    HRESULT LoadMovie (
        int layer,
        _bstr_t url );
    HRESULT TGotoFrame (
        _bstr_t target,
        long FrameNum );
    HRESULT TGotoLabel (
        _bstr_t target,
        _bstr_t label );
    long TCurrentFrame (
        _bstr_t target );
    _bstr_t TCurrentLabel (
        _bstr_t target );
    HRESULT TPlay (
        _bstr_t target );
    HRESULT TStopPlay (
        _bstr_t target );
    HRESULT SetVariable (
        _bstr_t name,
        _bstr_t value );
    _bstr_t GetVariable (
        _bstr_t name );
    HRESULT TSetProperty (
        _bstr_t target,
        int property,
        _bstr_t value );
    _bstr_t TGetProperty (
        _bstr_t target,
        int property );
    HRESULT TCallFrame (
        _bstr_t target,
        int FrameNum );
    HRESULT TCallLabel (
        _bstr_t target,
        _bstr_t label );
    HRESULT TSetPropertyNum (
        _bstr_t target,
        int property,
        double value );
    double TGetPropertyNum (
        _bstr_t target,
        int property );
    double TGetPropertyAsNumber (
        _bstr_t target,
        int property );
    _bstr_t GetSWRemote ( );
    void PutSWRemote (
        _bstr_t pVal );
    _bstr_t GetFlashVars ( );
    void PutFlashVars (
        _bstr_t pVal );
    _bstr_t GetAllowScriptAccess ( );
    void PutAllowScriptAccess (
        _bstr_t pVal );
    _bstr_t GetMovieData ( );
    void PutMovieData (
        _bstr_t pVal );
    IUnknownPtr GetInlineData ( );
    void PutInlineData (
        IUnknown * ppIUnknown );
    VARIANT_BOOL GetSeamlessTabbing ( );
    void PutSeamlessTabbing (
        VARIANT_BOOL pVal );
    HRESULT EnforceLocalSecurity ( );
    VARIANT_BOOL GetProfile ( );
    void PutProfile (
        VARIANT_BOOL pVal );
    _bstr_t GetProfileAddress ( );
    void PutProfileAddress (
        _bstr_t pVal );
    long GetProfilePort ( );
    void PutProfilePort (
        long pVal );
    _bstr_t CallFunction (
        _bstr_t request );
    HRESULT SetReturnValue (
        _bstr_t returnValue );
    HRESULT DisableLocalSecurity ( );
    _bstr_t GetAllowNetworking ( );
    void PutAllowNetworking (
        _bstr_t pVal );
    _bstr_t GetAllowFullScreen ( );
    void PutAllowFullScreen (
        _bstr_t pVal );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_ReadyState (
        /*[out,retval]*/ long * pVal ) = 0;
      virtual HRESULT __stdcall get_TotalFrames (
        /*[out,retval]*/ long * pVal ) = 0;
      virtual HRESULT __stdcall get_Playing (
        /*[out,retval]*/ VARIANT_BOOL * pVal ) = 0;
      virtual HRESULT __stdcall put_Playing (
        /*[in]*/ VARIANT_BOOL pVal ) = 0;
      virtual HRESULT __stdcall get_Quality (
        /*[out,retval]*/ int * pVal ) = 0;
      virtual HRESULT __stdcall put_Quality (
        /*[in]*/ int pVal ) = 0;
      virtual HRESULT __stdcall get_ScaleMode (
        /*[out,retval]*/ int * pVal ) = 0;
      virtual HRESULT __stdcall put_ScaleMode (
        /*[in]*/ int pVal ) = 0;
      virtual HRESULT __stdcall get_AlignMode (
        /*[out,retval]*/ int * pVal ) = 0;
      virtual HRESULT __stdcall put_AlignMode (
        /*[in]*/ int pVal ) = 0;
      virtual HRESULT __stdcall get_BackgroundColor (
        /*[out,retval]*/ long * pVal ) = 0;
      virtual HRESULT __stdcall put_BackgroundColor (
        /*[in]*/ long pVal ) = 0;
      virtual HRESULT __stdcall get_Loop (
        /*[out,retval]*/ VARIANT_BOOL * pVal ) = 0;
      virtual HRESULT __stdcall put_Loop (
        /*[in]*/ VARIANT_BOOL pVal ) = 0;
      virtual HRESULT __stdcall get_Movie (
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall put_Movie (
        /*[in]*/ BSTR pVal ) = 0;
      virtual HRESULT __stdcall get_FrameNum (
        /*[out,retval]*/ long * pVal ) = 0;
      virtual HRESULT __stdcall put_FrameNum (
        /*[in]*/ long pVal ) = 0;
      virtual HRESULT __stdcall raw_SetZoomRect (
        /*[in]*/ long left,
        /*[in]*/ long top,
        /*[in]*/ long right,
        /*[in]*/ long bottom ) = 0;
      virtual HRESULT __stdcall raw_Zoom (
        /*[in]*/ int factor ) = 0;
      virtual HRESULT __stdcall raw_Pan (
        /*[in]*/ long x,
        /*[in]*/ long y,
        /*[in]*/ int mode ) = 0;
      virtual HRESULT __stdcall raw_Play ( ) = 0;
      virtual HRESULT __stdcall raw_Stop ( ) = 0;
      virtual HRESULT __stdcall raw_Back ( ) = 0;
      virtual HRESULT __stdcall raw_Forward ( ) = 0;
      virtual HRESULT __stdcall raw_Rewind ( ) = 0;
      virtual HRESULT __stdcall raw_StopPlay ( ) = 0;
      virtual HRESULT __stdcall raw_GotoFrame (
        /*[in]*/ long FrameNum ) = 0;
      virtual HRESULT __stdcall raw_CurrentFrame (
        /*[out,retval]*/ long * FrameNum ) = 0;
      virtual HRESULT __stdcall raw_IsPlaying (
        /*[out,retval]*/ VARIANT_BOOL * Playing ) = 0;
      virtual HRESULT __stdcall raw_PercentLoaded (
        /*[out,retval]*/ long * percent ) = 0;
      virtual HRESULT __stdcall raw_FrameLoaded (
        /*[in]*/ long FrameNum,
        /*[out,retval]*/ VARIANT_BOOL * loaded ) = 0;
      virtual HRESULT __stdcall raw_FlashVersion (
        /*[out,retval]*/ long * version ) = 0;
      virtual HRESULT __stdcall get_WMode (
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall put_WMode (
        /*[in]*/ BSTR pVal ) = 0;
      virtual HRESULT __stdcall get_SAlign (
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall put_SAlign (
        /*[in]*/ BSTR pVal ) = 0;
      virtual HRESULT __stdcall get_Menu (
        /*[out,retval]*/ VARIANT_BOOL * pVal ) = 0;
      virtual HRESULT __stdcall put_Menu (
        /*[in]*/ VARIANT_BOOL pVal ) = 0;
      virtual HRESULT __stdcall get_Base (
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall put_Base (
        /*[in]*/ BSTR pVal ) = 0;
      virtual HRESULT __stdcall get_Scale (
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall put_Scale (
        /*[in]*/ BSTR pVal ) = 0;
      virtual HRESULT __stdcall get_DeviceFont (
        /*[out,retval]*/ VARIANT_BOOL * pVal ) = 0;
      virtual HRESULT __stdcall put_DeviceFont (
        /*[in]*/ VARIANT_BOOL pVal ) = 0;
      virtual HRESULT __stdcall get_EmbedMovie (
        /*[out,retval]*/ VARIANT_BOOL * pVal ) = 0;
      virtual HRESULT __stdcall put_EmbedMovie (
        /*[in]*/ VARIANT_BOOL pVal ) = 0;
      virtual HRESULT __stdcall get_BGColor (
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall put_BGColor (
        /*[in]*/ BSTR pVal ) = 0;
      virtual HRESULT __stdcall get_Quality2 (
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall put_Quality2 (
        /*[in]*/ BSTR pVal ) = 0;
      virtual HRESULT __stdcall raw_LoadMovie (
        /*[in]*/ int layer,
        /*[in]*/ BSTR url ) = 0;
      virtual HRESULT __stdcall raw_TGotoFrame (
        /*[in]*/ BSTR target,
        /*[in]*/ long FrameNum ) = 0;
      virtual HRESULT __stdcall raw_TGotoLabel (
        /*[in]*/ BSTR target,
        /*[in]*/ BSTR label ) = 0;
      virtual HRESULT __stdcall raw_TCurrentFrame (
        /*[in]*/ BSTR target,
        /*[out,retval]*/ long * FrameNum ) = 0;
      virtual HRESULT __stdcall raw_TCurrentLabel (
        /*[in]*/ BSTR target,
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall raw_TPlay (
        /*[in]*/ BSTR target ) = 0;
      virtual HRESULT __stdcall raw_TStopPlay (
        /*[in]*/ BSTR target ) = 0;
      virtual HRESULT __stdcall raw_SetVariable (
        /*[in]*/ BSTR name,
        /*[in]*/ BSTR value ) = 0;
      virtual HRESULT __stdcall raw_GetVariable (
        /*[in]*/ BSTR name,
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall raw_TSetProperty (
        /*[in]*/ BSTR target,
        /*[in]*/ int property,
        /*[in]*/ BSTR value ) = 0;
      virtual HRESULT __stdcall raw_TGetProperty (
        /*[in]*/ BSTR target,
        /*[in]*/ int property,
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall raw_TCallFrame (
        /*[in]*/ BSTR target,
        /*[in]*/ int FrameNum ) = 0;
      virtual HRESULT __stdcall raw_TCallLabel (
        /*[in]*/ BSTR target,
        /*[in]*/ BSTR label ) = 0;
      virtual HRESULT __stdcall raw_TSetPropertyNum (
        /*[in]*/ BSTR target,
        /*[in]*/ int property,
        /*[in]*/ double value ) = 0;
      virtual HRESULT __stdcall raw_TGetPropertyNum (
        /*[in]*/ BSTR target,
        /*[in]*/ int property,
        /*[out,retval]*/ double * pVal ) = 0;
      virtual HRESULT __stdcall raw_TGetPropertyAsNumber (
        /*[in]*/ BSTR target,
        /*[in]*/ int property,
        /*[out,retval]*/ double * pVal ) = 0;
      virtual HRESULT __stdcall get_SWRemote (
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall put_SWRemote (
        /*[in]*/ BSTR pVal ) = 0;
      virtual HRESULT __stdcall get_FlashVars (
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall put_FlashVars (
        /*[in]*/ BSTR pVal ) = 0;
      virtual HRESULT __stdcall get_AllowScriptAccess (
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall put_AllowScriptAccess (
        /*[in]*/ BSTR pVal ) = 0;
      virtual HRESULT __stdcall get_MovieData (
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall put_MovieData (
        /*[in]*/ BSTR pVal ) = 0;
      virtual HRESULT __stdcall get_InlineData (
        /*[out,retval]*/ IUnknown * * ppIUnknown ) = 0;
      virtual HRESULT __stdcall put_InlineData (
        /*[in]*/ IUnknown * ppIUnknown ) = 0;
      virtual HRESULT __stdcall get_SeamlessTabbing (
        /*[out,retval]*/ VARIANT_BOOL * pVal ) = 0;
      virtual HRESULT __stdcall put_SeamlessTabbing (
        /*[in]*/ VARIANT_BOOL pVal ) = 0;
      virtual HRESULT __stdcall raw_EnforceLocalSecurity ( ) = 0;
      virtual HRESULT __stdcall get_Profile (
        /*[out,retval]*/ VARIANT_BOOL * pVal ) = 0;
      virtual HRESULT __stdcall put_Profile (
        /*[in]*/ VARIANT_BOOL pVal ) = 0;
      virtual HRESULT __stdcall get_ProfileAddress (
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall put_ProfileAddress (
        /*[in]*/ BSTR pVal ) = 0;
      virtual HRESULT __stdcall get_ProfilePort (
        /*[out,retval]*/ long * pVal ) = 0;
      virtual HRESULT __stdcall put_ProfilePort (
        /*[in]*/ long pVal ) = 0;
      virtual HRESULT __stdcall raw_CallFunction (
        /*[in]*/ BSTR request,
        /*[out,retval]*/ BSTR * response ) = 0;
      virtual HRESULT __stdcall raw_SetReturnValue (
        /*[in]*/ BSTR returnValue ) = 0;
      virtual HRESULT __stdcall raw_DisableLocalSecurity ( ) = 0;
      virtual HRESULT __stdcall get_AllowNetworking (
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall put_AllowNetworking (
        /*[in]*/ BSTR pVal ) = 0;
      virtual HRESULT __stdcall get_AllowFullScreen (
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall put_AllowFullScreen (
        /*[in]*/ BSTR pVal ) = 0;
};

struct __declspec(uuid("c5598e60-b307-11d1-b27d-006008c3fbfb"))
ICanHandleException : IUnknown
{
    //
    // Wrapper methods for error-handling
    //

    HRESULT CanHandleException (
        EXCEPINFO * pExcepInfo,
        VARIANT * pvar );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall raw_CanHandleException (
        /*[in]*/ EXCEPINFO * pExcepInfo,
        /*[in]*/ VARIANT * pvar ) = 0;
};

struct __declspec(uuid("d27cdb6d-ae6d-11cf-96b8-444553540000"))
_IShockwaveFlashEvents : IDispatch
{
    //
    // Wrapper methods for error-handling
    //

    // Methods:
    HRESULT OnReadyStateChange (
        long newState );
    HRESULT OnProgress (
        long percentDone );
    HRESULT FSCommand (
        _bstr_t command,
        _bstr_t args );
    HRESULT FlashCall (
        _bstr_t request );
};

struct __declspec(uuid("d27cdb6e-ae6d-11cf-96b8-444553540000"))
ShockwaveFlash;
    // [ default ] interface IShockwaveFlash
    // [ default, source ] dispinterface _IShockwaveFlashEvents

struct __declspec(uuid("d27cdb70-ae6d-11cf-96b8-444553540000"))
IFlashFactory : IUnknown
{};

struct __declspec(uuid("a6ef9860-c720-11d0-9337-00a0c90dcaa9"))
IDispatchEx : IDispatch
{
    //
    // Wrapper methods for error-handling
    //

    HRESULT GetDispID (
        _bstr_t bstrName,
        unsigned long grfdex,
        long * pid );
    HRESULT RemoteInvokeEx (
        long id,
        unsigned long lcid,
        unsigned long dwFlags,
        DISPPARAMS * pdp,
        VARIANT * pvarRes,
        EXCEPINFO * pei,
        struct IServiceProvider * pspCaller,
        unsigned int cvarRefArg,
        unsigned int * rgiRefArg,
        VARIANT * rgvarRefArg );
    HRESULT DeleteMemberByName (
        _bstr_t bstrName,
        unsigned long grfdex );
    HRESULT DeleteMemberByDispID (
        long id );
    HRESULT GetMemberProperties (
        long id,
        unsigned long grfdexFetch,
        unsigned long * pgrfdex );
    HRESULT GetMemberName (
        long id,
        BSTR * pbstrName );
    HRESULT GetNextDispID (
        unsigned long grfdex,
        long id,
        long * pid );
    HRESULT GetNameSpaceParent (
        IUnknown * * ppunk );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall raw_GetDispID (
        /*[in]*/ BSTR bstrName,
        /*[in]*/ unsigned long grfdex,
        /*[out]*/ long * pid ) = 0;
      virtual HRESULT __stdcall raw_RemoteInvokeEx (
        /*[in]*/ long id,
        /*[in]*/ unsigned long lcid,
        /*[in]*/ unsigned long dwFlags,
        /*[in]*/ DISPPARAMS * pdp,
        /*[out]*/ VARIANT * pvarRes,
        /*[out]*/ EXCEPINFO * pei,
        /*[in]*/ struct IServiceProvider * pspCaller,
        /*[in]*/ unsigned int cvarRefArg,
        /*[in]*/ unsigned int * rgiRefArg,
        /*[in,out]*/ VARIANT * rgvarRefArg ) = 0;
      virtual HRESULT __stdcall raw_DeleteMemberByName (
        /*[in]*/ BSTR bstrName,
        /*[in]*/ unsigned long grfdex ) = 0;
      virtual HRESULT __stdcall raw_DeleteMemberByDispID (
        /*[in]*/ long id ) = 0;
      virtual HRESULT __stdcall raw_GetMemberProperties (
        /*[in]*/ long id,
        /*[in]*/ unsigned long grfdexFetch,
        /*[out]*/ unsigned long * pgrfdex ) = 0;
      virtual HRESULT __stdcall raw_GetMemberName (
        /*[in]*/ long id,
        /*[out]*/ BSTR * pbstrName ) = 0;
      virtual HRESULT __stdcall raw_GetNextDispID (
        /*[in]*/ unsigned long grfdex,
        /*[in]*/ long id,
        /*[out]*/ long * pid ) = 0;
      virtual HRESULT __stdcall raw_GetNameSpaceParent (
        /*[out]*/ IUnknown * * ppunk ) = 0;
};

struct __declspec(uuid("d27cdb72-ae6d-11cf-96b8-444553540000"))
IFlashObjectInterface : IDispatchEx
{};

struct __declspec(uuid("d27cdb71-ae6d-11cf-96b8-444553540000"))
FlashObjectInterface;
    // [ default ] interface IFlashObjectInterface

struct __declspec(uuid("86230738-d762-4c50-a2de-a753e5b1686f"))
IFlashObject : IDispatchEx
{};

struct __declspec(uuid("e0920e11-6b65-4d5d-9c58-b1fc5c07dc43"))
FlashObject;
    // [ default ] interface IFlashObject

//
// Named GUID constants initializations
//

extern "C" const GUID __declspec(selectany) LIBID_ShockwaveFlashObjects =
    {0xd27cdb6b,0xae6d,0x11cf,{0x96,0xb8,0x44,0x45,0x53,0x54,0x00,0x00}};
extern "C" const GUID __declspec(selectany) IID_IShockwaveFlash =
    {0xd27cdb6c,0xae6d,0x11cf,{0x96,0xb8,0x44,0x45,0x53,0x54,0x00,0x00}};
extern "C" const GUID __declspec(selectany) IID_ICanHandleException =
    {0xc5598e60,0xb307,0x11d1,{0xb2,0x7d,0x00,0x60,0x08,0xc3,0xfb,0xfb}};
extern "C" const GUID __declspec(selectany) DIID__IShockwaveFlashEvents =
    {0xd27cdb6d,0xae6d,0x11cf,{0x96,0xb8,0x44,0x45,0x53,0x54,0x00,0x00}};
extern "C" const GUID __declspec(selectany) CLSID_ShockwaveFlash =
    {0xd27cdb6e,0xae6d,0x11cf,{0x96,0xb8,0x44,0x45,0x53,0x54,0x00,0x00}};
extern "C" const GUID __declspec(selectany) IID_IFlashFactory =
    {0xd27cdb70,0xae6d,0x11cf,{0x96,0xb8,0x44,0x45,0x53,0x54,0x00,0x00}};
extern "C" const GUID __declspec(selectany) IID_IDispatchEx =
    {0xa6ef9860,0xc720,0x11d0,{0x93,0x37,0x00,0xa0,0xc9,0x0d,0xca,0xa9}};
extern "C" const GUID __declspec(selectany) IID_IFlashObjectInterface =
    {0xd27cdb72,0xae6d,0x11cf,{0x96,0xb8,0x44,0x45,0x53,0x54,0x00,0x00}};
extern "C" const GUID __declspec(selectany) CLSID_FlashObjectInterface =
    {0xd27cdb71,0xae6d,0x11cf,{0x96,0xb8,0x44,0x45,0x53,0x54,0x00,0x00}};
extern "C" const GUID __declspec(selectany) IID_IFlashObject =
    {0x86230738,0xd762,0x4c50,{0xa2,0xde,0xa7,0x53,0xe5,0xb1,0x68,0x6f}};
extern "C" const GUID __declspec(selectany) CLSID_FlashObject =
    {0xe0920e11,0x6b65,0x4d5d,{0x9c,0x58,0xb1,0xfc,0x5c,0x07,0xdc,0x43}};

//
// Wrapper method implementations
//

//#include "e:\vs_workspace\switch_sf\release\flash10i.tli"

} // namespace ShockwaveFlashObjects

#pragma pack(pop)
#pragma warning(default: 4192)

using namespace ShockwaveFlashObjects;


typedef HRESULT (__stdcall *DllGetClassObjectFunc)(REFCLSID rclsid, REFIID riid, LPVOID * ppv);
