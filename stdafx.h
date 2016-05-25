// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef STRICT
#define STRICT
#endif

#include "targetver.h"

#define _ATL_APARTMENT_THREADED

#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit


#define ATL_NO_ASSERT_ON_DESTROY_NONEXISTENT_WINDOW

#include "resource.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>
#include <atlstr.h>

#import <msxml6.dll> no_function_mapping
#define XMLHTTP_COMPONENT L"Msxml2.ServerXMLHTTP.6.0"
// #define XMLHTTP_COMPONENT L"Msxml2.XMLHTTP.6.0"
// #define XMLHTTP_COMPONENT __uuidof(MSXML2::ServerXMLHTTP60)

