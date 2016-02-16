#include "stdafx.h"
#include "SimulatorService.h"

/*static*/ const CString CSimulatorWf::FileMonikerDN4TokenResponse(LPCTSTR szClientId)
{
	CString strMonikerName;
	strMonikerName.Format(TOKEN_RESPONSE_USER_ENV_FMT, szClientId);

	static CString strResult;
	// das PathUnExpandEnvStrings() ist die RICHTIGE funktion NUR sie funktioniert NICHT???
	// if (FALSE == ::PathUnExpandEnvStringsW(L"%appdata%\\client_secret_<ClientId>.TokenResponse-user", szExpand, _countof(szExpand))) // %APPDATA%, %USERPROFILE%, %cticlientappdata%
	if (0 == ::ExpandEnvironmentStrings((LPCTSTR)strMonikerName, strResult.GetBufferSetLength(MAX_PATH), MAX_PATH))
	{
		const DWORD dwLastError = ::GetLastError();
		HRESULT hr = HRESULT_FROM_WIN32(dwLastError);
		_ASSERT(FALSE); // das kann nach menschlichem ermessen NICHT fehlschlagen
	}

	strResult.ReleaseBuffer();
	return strResult;
}

/*static*/ HRESULT CSimulatorWf::GetTokenServerByDisplayName(LPCTSTR szDisplayName, oAuthLib::IAuthorize** ppAuthorize)
{
	BIND_OPTS2 bo;
	bo.cbStruct = sizeof(BIND_OPTS2);
	bo.grfFlags = 0;
	bo.grfMode = STGM_READ | STGM_CONVERT;
	bo.dwTickCountDeadline = 0;
	bo.dwTrackFlags = 0;
	// (default) wir binden die DLL klasse als DLLSurrogat, CLSCTX_LOCAL_SERVER eintragen
	// zum debuggen der klasse hier CLSCTX_ALL oder gleich CLSCTX_INPROC_SERVER eintragen
	//   jetzt kann man einfach beliebige breakpoints in der oAuthapplication setzen.
	//   Hinweis: Application terminiert nicht mehr!!! (es loest ja keiner mehr den lock)
	bo.dwClassContext = CLSCTX_LOCAL_SERVER;
	bo.locale = LOCALE_USER_DEFAULT;
	bo.pServerInfo = NULL;
	return ::CoGetObject(szDisplayName, &bo, IID_PPV_ARGS(ppAuthorize));
}

IMPLEMENT_DYNCREATE(CSimulatorPing, CoAuthServiceCall)

HRESULT CSimulatorPing::Init(LPCTSTR szClientId)
{
	return __super::Init(_T("GET"), _T("http://ws-psi-win8.estos.de/procallsdk/solution/teamviewer/tvsimulator/ping.asp"));
}

/*virtual*/ HRESULT CSimulatorPing::GetTokenServer(oAuthLib::IAuthorize** ppAuthorize)
{
	const CString strMonikerName = CSimulatorWf::FileMonikerDN4TokenResponse(_T("SimulatorClientId"));
	return CSimulatorWf::GetTokenServerByDisplayName(strMonikerName, ppAuthorize);
}

/*virtual*/ void CSimulatorPing::onSucceeded(web::json::value& result)
{
	TRACE1("  result: %s\n", result[_T("result")].as_string().c_str());
}
