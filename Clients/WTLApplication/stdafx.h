// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//  are changed infrequently
//

#pragma once

// Change these values to use different versions
#define WINVER			0x0500
#define _WIN32_WINNT	0x0501
#define _WIN32_IE		0x0501
#define _RICHEDIT_VER	0x0500

#include <atlbase.h>
#include <atlstr.h> // muss VOR atlapp.h
#include <atlapp.h>
#include <atlctrls.h>

extern CAppModule _Module;

#include <atlwin.h>

#if defined _M_IX86
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

// the following are needed to use "AuthorizationServer"
// #define AUTHORIZATION_SERVER_SUPPORT_JSON
#ifdef AUTHORIZATION_SERVER_SUPPORT_JSON
// C++ REST SDK (Codename "Casablanca")
//   NuGet Package, Version 2.7.0 (desktop), https://github.com/Microsoft/cpprestsdk
//   most of the web payload is json encoded
// CAUTION: this is necessary to avoid false memory leaks reported by the MFC framework;
// http://codexpert.ro/blog/2015/05/23/using-lambdas-in-mfc-applications-part-3-dealing-with-c-rest-sdk/
#include <cpprest/json.h>
#endif

// were using XMLHTTPRequest for network access
#import <msxml6.dll> no_function_mapping
#define XMLHTTP_COMPONENT L"Msxml2.ServerXMLHTTP.6.0"
// #define XMLHTTP_COMPONENT L"Msxml2.XMLHTTP.6.0"
// #define XMLHTTP_COMPONENT __uuidof(MSXML2::ServerXMLHTTP60)

// the "AuthrizationServer" itself
#ifdef _DEBUG
#import "..\..\Debug\oAuth.tlb"
#else
#import "..\..\Release\oAuth.tlb"
#endif
