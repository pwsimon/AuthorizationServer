// MFCApplication.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "MFCApplication_i.h"

// CMFCApplicationApp:
// See MFCApplication.cpp for the implementation of this class
//
class CMFCApplicationApp : public CWinApp
{
public:
	CMFCApplicationApp();

// Overrides
public:
	virtual BOOL InitInstance();
	BOOL ExitInstance();

// Implementation
	DECLARE_MESSAGE_MAP()
};

extern CMFCApplicationApp theApp;
