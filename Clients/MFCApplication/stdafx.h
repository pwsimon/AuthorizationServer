
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#define _ATL_APARTMENT_THREADED 
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#include <afxdisp.h>        // MFC Automation classes

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // MFC support for ribbons and control bars

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

// the following are needed to use "AuthorizationServer"
#define AUTHORIZATION_SERVER_SUPPORT_JSON
#ifdef AUTHORIZATION_SERVER_SUPPORT_JSON
	// C++ REST SDK (Codename "Casablanca")
	//   NuGet Package, Version 2.7.0 (desktop), https://github.com/Microsoft/cpprestsdk
	//   most of the web payload is json encoded
	// CAUTION: this is necessary to avoid false memory leaks reported by the MFC framework;
	// http://codexpert.ro/blog/2015/05/23/using-lambdas-in-mfc-applications-part-3-dealing-with-c-rest-sdk/
	#include <cpprest/json.h>
#endif

// were using XMLHTTPRequest for network access
#import <msxml4.dll> no_function_mapping

// the "AuthrizationServer" itself
#ifdef _DEBUG
	#import "..\..\Debug\oAuth.tlb"
#else
	#import "..\..\Release\oAuth.tlb"
#endif
