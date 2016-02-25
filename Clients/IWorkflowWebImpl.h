// IWorkflowWebImpl.h : Declaration of IWorkflowWebImpl

#pragma once

/*
* die idee an sich ist aus meiner sicht nicht so schlecht denn so koennte man auch den <company>/<product> speziellen pfad abhandeln.
* machen wir z.b. mit dem Meta auch so
*   ::SetEnvironmentVariable(_T("cticlientlocalappdata"), theServerSettings.GetCtiClientApplicationLocalDirectory().c_str());
*   ::SetEnvironmentVariable(_T("cticlientappdata"), theServerSettings.GetCtiClientApplicationRoamingDirectory().c_str());
*
* - die ansicht: voellig auf absolute FileNamen zu verzichten und alles relativ zu addressieren,
*   koennen wir getrost "ad acta" legen denn: es KANN NICHT in ein %ProgramFiles(x86)% verzeichnis geschrieben werden
*
* - andererseits gibt es inzwischen vermehrt produkte (spotify, chrome) die sich explizit lokal/per user installieren
* mal ein "per user" .msi bauen und einen pfad unter %appdata% installieren
*/
#define TOKEN_RESPONSE_USER_ENV_FMT	_T("%%appdata%%\\client_secret_%s.TokenResponse-user")
#define CLIENT_SECRET_ENV_FMT		_T("%%appdata%%\\client_secret_%s.json")

template < class T >
class ATL_NO_VTABLE IWorkflowWebImpl
{
public:
	IWorkflowWebImpl()
	{
	}

	virtual ~IWorkflowWebImpl()
	{
	}

/*
* eine simple komfortfunktion die AUSSCHLIESSLICH die aufgabe hat das Namensschema einheitlich zu halten.
* - wenn das letztlich nervt das hier der komplette ::theServerSettings drinnhaengt dann gnadenlos rausschmeissen.
* - in KEINEM fall hier eine MemberFunktion daraus machen
*/
	static LPCTSTR FileMonikerDN4TokenResponse(LPCTSTR szClientId)
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

	static LPCTSTR FileMonikerDN4ClientSecret(LPCTSTR szClientId)
	{
		CString strMonikerName;
		strMonikerName.Format(CLIENT_SECRET_ENV_FMT, szClientId);

		static CString strResult;
		// das PathUnExpandEnvStrings() ist die RICHTIGE funktion NUR sie funktioniert NICHT???
		// if (FALSE == ::PathUnExpandEnvStringsW(L"%appdata%\\client_secret_<ClientId>.json", szExpand, _countof(szExpand))) // %APPDATA%, %USERPROFILE%, %cticlientappdata%
		if (0 == ::ExpandEnvironmentStrings((LPCTSTR)strMonikerName, strResult.GetBufferSetLength(MAX_PATH), MAX_PATH))
		{
			const DWORD dwLastError = ::GetLastError();
			HRESULT hr = HRESULT_FROM_WIN32(dwLastError);
			_ASSERT(FALSE); // das kann nach menschlichem ermessen NICHT fehlschlagen
		}

		strResult.ReleaseBuffer();
		return strResult;
	}

	static HRESULT GetTokenServerByDisplayName(LPCTSTR szDisplayName, oAuthLib::IAuthorize** ppAuthorize)
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
};
