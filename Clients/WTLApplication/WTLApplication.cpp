// WTLApplication.cpp : main source file for WTLApplication.exe
//

#include "stdafx.h"

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>

#include "resource.h"

#include "MainDlg.h"

CAppModule _Module;

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int /*nCmdShow*/)
{
	HRESULT hRes = ::CoInitialize(NULL);
	/* If you are running on NT 4.0 or higher you can use the following call instead to 
	* make the EXE free threaded. This means that calls come in on a random RPC thread.
	* HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	*/
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES); // add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = 0;
	// BLOCK: Run application
	{
		CMainDlg dlgMain(lpstrCmdLine);
		nRet = dlgMain.DoModal();
#ifdef FEATURE_TASKSCHD
		/*
		* wenn ich den returnCode von DoModal() uebernehmen wollte muesste ich JEDEN exit ueberschreiben
		* damit ICH den wert von EndDialog() kontrollieren kann. OnCancel(), OnOk(), OnClose() is mir zuviel
		*
		* TracePointDef: {dlgMain.m_iExitCode} = _tWinMain()
		* Labels/Keywords:
		*/
		nRet = dlgMain.m_iExitCode;
#endif
	}

	_Module.Term();
	::CoUninitialize();

#ifdef FEATURE_TASKSCHD
	::ExitProcess(nRet);
#endif
	return nRet;
}
