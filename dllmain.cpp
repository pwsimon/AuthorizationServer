// dllmain.cpp : Implementation of DllMain.

#include "stdafx.h"
#include "resource.h"
#include "oAuth_i.h"
#include "dllmain.h"

CoAuthModule _AtlModule;

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	hInstance;
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		BOOL bRetC = _AtlModule.DllMain(dwReason, lpReserved);
		EventRegisterPRIVATE_PWSIMON_DEV_AuthorizationServer();
		return bRetC;
	}

	else if (dwReason == DLL_PROCESS_DETACH)
	{
		EventUnregisterPRIVATE_PWSIMON_DEV_AuthorizationServer();
		return _AtlModule.DllMain(dwReason, lpReserved);
	}

	return _AtlModule.DllMain(dwReason, lpReserved);
}
