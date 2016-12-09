// oAuth.cpp : Implementation of DLL Exports.

#include "stdafx.h"
#include "resource.h"
#include "oAuth_i.h"
#include "dllmain.h"

using namespace ATL;

// Used to determine whether the DLL can be unloaded by OLE.
STDAPI DllCanUnloadNow(void)
{
	return _AtlModule.DllCanUnloadNow();
}

// Returns a class factory to create an object of the requested type.
STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID* ppv)
{
	return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

// DllRegisterServer - Adds entries to the system registry.
STDAPI DllRegisterServer(void)
{
	// registers object, typelib and all interfaces in typelib
	HRESULT hr = _AtlModule.DllRegisterServer();

	/*
	* Configuring and Starting an Event Tracing Session, https://msdn.microsoft.com/en-us/library/windows/desktop/aa363688(v=vs.85).aspx
	* C:\Windows\System32\wevtutil.exe /?
	* C:\Windows\System32\wevtutil ep | more           // enum installed providers
	* C:\Windows\System32\wevtutil.exe im oAuth.man    // install manifest, ACHTUNG das modul MUSS auf dem %SystemDrive% liegen sonst werden die resourcen NICHT gefunden ???
	* C:\Windows\System32\wevtutil.exe um oAuth.man    // uninstall manifest
	* C:\Windows\System32\wevtutil.exe qe Application  // query for events from "Application"
	*/
	// EnableTraceEx2 function, https://msdn.microsoft.com/en-us/library/windows/desktop/dd392305(v=vs.85).aspx
	// EnableTraceEx(&ESTOS_PROCALL_UC_PRESENCEPROVIDER, NULL,);

	/*
	* About Event Tracing, https://msdn.microsoft.com/en-us/library/windows/desktop/aa363668(v=vs.85).aspx
	* NOT Event Logging, https://msdn.microsoft.com/en-us/library/windows/desktop/aa363652(v=vs.85).aspx // Prior to Windows Vista
	* NOT TraceLogging, https://msdn.microsoft.com/en-us/library/windows/desktop/dn904636(v=vs.85).aspx // Windows 10
	*
	* How to: Add Custom Build Tools to MSBuild Projects, http://msdn.microsoft.com/en-us/library/dd293705.aspx
	* Writing an Instrumentation Manifest, https://msdn.microsoft.com/en-us/library/windows/desktop/dd996930(v=vs.85).aspx
	* "C:\Program Files (x86)\Windows Kits\10\bin\x64\ECManGen.exe"
	* https://msdn.microsoft.com/en-us/library/windows/desktop/aa385638(v=vs.85).aspx
	* "C:\Program Files (x86)\Windows Kits\10\bin\x64\mc.exe" -v -um -z oAuthEvents oAuth.man
	* Wevtutil
	*
	* Runtime: (Aktive Event Tracing Sessions kann man über den Performance Monitor einsehen)
	*   Configuring and Starting an Event Tracing Session, https://msdn.microsoft.com/en-us/library/windows/desktop/aa363688(v=vs.85).aspx
	*   siehe auch: "RegServer"
	*/

	return hr;
}

// DllUnregisterServer - Removes entries from the system registry.
STDAPI DllUnregisterServer(void)
{
	HRESULT hr = _AtlModule.DllUnregisterServer();
	return hr;
}

// DllInstall - Adds/Removes entries to the system registry per user per machine.
STDAPI DllInstall(BOOL bInstall, _In_opt_  LPCWSTR pszCmdLine)
{
	HRESULT hr = E_FAIL;
	static const wchar_t szUserSwitch[] = L"user";

	if (pszCmdLine != NULL)
	{
		if (_wcsnicmp(pszCmdLine, szUserSwitch, _countof(szUserSwitch)) == 0)
		{
			ATL::AtlSetPerUserRegistration(true);
		}
	}

	if (bInstall)
	{
		hr = DllRegisterServer();
		if (FAILED(hr))
		{
			DllUnregisterServer();
		}
	}
	else
	{
		hr = DllUnregisterServer();
	}

	return hr;
}
