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
	return E_NOTIMPL;
}

IMPLEMENT_DYNCREATE(CSimulatorPing, CoAuthServiceCall)

HRESULT CSimulatorPing::Init()
{
	return __super::Init(CComBSTR(L"http://ws-psi-win8.estos.de/procallsdk/solution/teamviewer/tvsimulator/ping.asp"));
}

/*virtual*/ HRESULT CSimulatorPing::GetTokenServer(oAuthLib::IAuthorize** ppAuthorize)
{
	const CString strMonikerName = CSimulatorWf::FileMonikerDN4TokenResponse(_T("SimulatorClientId"));
	return CSimulatorWf::GetTokenServerByDisplayName(strMonikerName, ppAuthorize);
}
