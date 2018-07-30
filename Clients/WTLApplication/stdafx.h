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

/*
* wir fuehren eine liste der requests um diese aktiv canceln (IXMLHTTPRequest::abort()) zu koennen
*/
#define MONITOR_PENDING_REQUESTS
#ifdef MONITOR_PENDING_REQUESTS
#include <list>
#endif

/*
* das periodische wiederholen auf basis des Task Scheduler (TASK_ACTION_EXEC) bedeutet aktuell u.A.
* das wir die "geteilten" credentials auch auf ::CreateFile(FILE_SHARE_READ) sperren bzw. aktualisieren koennten ...
* ganz generell denke ich ist es ausschliesslich sinnvoll mit TASK_ACTION_EXEC nur rein auf BATCH verarbeitung getrimmte anwendungen zu starten.
* hier kann man dann auch ganz einfach einen returncode einbauen der dann auch im Task Scheduler log auftaucht
* siehe auch:
*   
*   powercfg /a, zeigt die auf dem jeweiligen system verfuegbaren "sleep states"
*   Windows 10 task scheduler will not wake computer, https://social.technet.microsoft.com/Forums/office/en-US/c39f3759-76ab-4712-ae09-7ee102a94b9a/windows-10-task-scheduler-will-not-wake-computer?forum=win10itprogeneral
*/
#define FEATURE_TASKSCHD

/*
* auf basis eines "CreateWaitableTimer function" koennen wir eine application "schlafen" legen.
* das OS garantiert das mit dem ablauf des times der PC, sofern noetig, ein Wake up ausfuehrt.
* System Wake-up Events, https://msdn.microsoft.com/en-us/library/windows/desktop/aa373235(v=vs.85).aspx
* ToDo:
*   need first implementation
*/
// #define FEATURE_WAITABLETIMER

#include <atlbase.h>
#include <atlstr.h> // muss VOR atlapp.h
#include <atlapp.h>
#include <atlctrls.h>
#include <ATLComTime.h>

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

/*
* WinHttp::IWinHttpRequest
#define XMLHTTP_WINHTTP
*/
#ifdef XMLHTTP_WINHTTP
	#import <winhttpcom.dll> no_function_mapping
#endif

/*
* auch das mshtml bringt ein XHR Object mit nur ich kann KEINE instance erzeugen
#define XMLHTTP_MSHTML_DOMAINREQUEST
*/
#ifdef XMLHTTP_MSHTML_DOMAINREQUEST
	#import <mshtml.tlb> no_function_mapping
#endif

/*
* die msxml3.dll bzw. dessen TypeLib kennt KEIN IXMLHTTPRequest2
* die msxml3.dll MUSS ab windows10 manuell nach installiert werden
#import <msxml3.dll> no_function_mapping
#define XMLHTTP_COMPONENT "Msxml2.XMLHTTP.3.0"
*/

/*
* were using XMLHTTPRequest for network access
* serverseitig kann man die verwendete client componente am User-Agent-Header erkennen
*   User-Agent-Header, https://de.wikipedia.org/wiki/User_Agent
* "Msxml2.ServerXMLHTTP.6.0" -> "Mozilla/4.0 (compatible; Win32; WinHttp.WinHttpRequest.5)"
* "Msxml2.XMLHTTP.6.0"       -> "Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 6.2; WOW64; Trident/7.0; .NET4.0C; .NET4.0E; .NET CLR 2.0.50727; .NET CLR 3.0.30729; .NET CLR 3.5.30729)"
*/
#import <msxml6.dll> no_function_mapping
#define XMLHTTP_SERVERREQUEST
#define XMLHTTP_COMPONENT L"Msxml2.ServerXMLHTTP.6.0"
// #define XMLHTTP_COMPONENT __uuidof(MSXML2::ServerXMLHTTP60)
// #define XMLHTTP_COMPONENT L"Msxml2.XMLHTTP.6.0"

/*
* das msxml6.dll bringt MEHRERE XHR Object mit. im besonderen eines das auch fuer UWP's geeignet ist
#define XMLHTTP_FREETHREADEDXMLHTTP60
*/

// the "AuthorizationServer" itself
#ifdef _DEBUG
#import "..\..\Debug\oAuth.tlb"
#else
#import "..\..\Release\oAuth.tlb"
#endif
