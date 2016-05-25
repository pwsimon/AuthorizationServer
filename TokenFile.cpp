// TokenFile.cpp : Implementation of CTokenFile

#include "stdafx.h"
#include "TokenFile.h"

// CTokenFile
// Registering the DLL Server for Surrogate Activation
//   https://msdn.microsoft.com/en-us/library/windows/desktop/ms686606(v=vs.85).aspx
// DllSurrogate
//   https://msdn.microsoft.com/en-us/library/ms691260(v=vs.85).aspx
// RunAs (using dcomcnfg)
//   https://msdn.microsoft.com/en-us/library/windows/desktop/ms680046(v=vs.85).aspx
HRESULT CTokenFile::FinalConstruct()
{
	return NOERROR;
}

/*
* return values
*   S_OK,         file geoeffnet mit tokens
*   E_FAIL,       file not well formed, ...
*   S_FALSE,      file not found, can not open, ...
*/
HRESULT CTokenFile::LoadTokenResponseFromFile(
	LPCOLESTR pszFileName,
	DWORD dwMode)
{
	std::fstream is(pszFileName, std::fstream::in);

	/*
	* die pruefung auf open ist in unserem fall eigentlich ueberfluessig denn sie kann per definition NICHT auftreten
	* wir erzeugen eine instance von CTokenFile ausschliesslich indirekt ueber ::CoGetObject(<FileName>.TokenResponse-user)
	* der FileMoniker prueft VOR dem erzeugen schon auf "existence" im fehlerfall wird gar keine instance erzeugt.
	* was ist aber wenn:
	* - die zugriffsrechte nicht gegeben sind
	* - die datei empty ist (web::json::value::parse throws exception)
	* - und und und ...
	* http://www.cplusplus.com/reference/ios/ios/rdstate/
	*/
	if (is.is_open())
	{
		// https://casablanca.codeplex.com/wikipage?title=JSON&referringTitle=Documentation
		web::json::value root = web::json::value::parse(is);
		m_strAccessToken = root[_T("access_token")].as_string().c_str();
		m_strRefreshToken = root[_T("refresh_token")].as_string().c_str();
		m_strTokenType = root[_T("token_type")].as_string().c_str();
		is.close();
		return NOERROR;
	}
	else
		return E_FAIL;
}

/*
* persist all membervariables to file.
*/
HRESULT CTokenFile::UpdateAccessRefreshToken(
	BSTR bstrResponseText)
{
	// argumente extrahieren
	web::json::value root = web::json::value::parse(bstrResponseText);

	m_strAccessToken = root[_T("access_token")].as_string().c_str();
	m_strRefreshToken = root[_T("refresh_token")].as_string().c_str();
	m_strTokenType = root[_T("token_type")].as_string().c_str();

	// IPersistFile::Save method, https://msdn.microsoft.com/en-us/library/windows/desktop/ms693701(v=vs.85).aspx
	// The absolute path of the file to which the object should be saved. If pszFileName is NULL, the object should save its data to the current file, if there is one.
	return Save(NULL, false);
}

// IPersistFile implementation
STDMETHODIMP CTokenFile::Load(
	/* [in] */ __RPC__in LPCOLESTR pszFileName,
	/* [in] */ DWORD dwMode)
{
	ATLTRACE2(atlTraceRefcount, 0, _T("CTokenFile(%ls)::IPersistFile::Load()\n"), pszFileName);

	try
	{
		TCHAR szClientSecretFileName[MAX_PATH];
		wcscpy_s(szClientSecretFileName, MAX_PATH, pszFileName);
		::PathRenameExtension(szClientSecretFileName, _T(".json"));
#ifdef _DEBUG
		const CString strFileName = ::PathFindFileName(pszFileName);
		const int iPrefixPos = strFileName.Find(_T("client_secret_"));
		const int iExtensionPos = strFileName.Find(_T(".TokenResponse-user"));
		const CString strClientId = strFileName.Mid(iPrefixPos + 14, iExtensionPos - iPrefixPos - 14);
#endif

		// How to: Work with JSON Data (C++ REST SDK)
		//   https://msdn.microsoft.com/en-us/library/jj950082.aspx
		// diese configuration/settings werden aus der "%appdata%/client_secret_<ClientId>.json" geladen
		// 
		// dieses file wird durch download als ergebnis des registrierungs prozesses durch die entsprechende "Developer-Console" bereitgestellt.
		{
			std::fstream is(szClientSecretFileName);
			web::json::value root = web::json::value::parse(is);

			m_strClientId = root[_T("installed")][_T("client_id")].as_string().c_str();
			m_strSecret = root[_T("installed")][_T("client_secret")].as_string().c_str();

			// durch diese diese information ist diese implementierung vom der API/Hersteller unabhaengig
			m_strAuthUri = root[_T("installed")][_T("auth_uri")].as_string().c_str(); // "https://webapi.teamviewer.com/api/v1/oauth2/authorize"
			m_strTokenUri = root[_T("installed")][_T("token_uri")].as_string().c_str();

			// im configfile werden ALLE (multivalue) moeglichen uris aufgelistet
			// wir waehlen einen ganz bestimmten. siehe unten:
			// CString strRedirectUri(installed["redirect_uris"].asCString());
		}

		// die configuration/settings werden (sofern vorhanden) aus der "%appdata%/client_secret_<ClientId>.TokenResponse-user" geladen
		HRESULT hr = LoadTokenResponseFromFile(pszFileName, dwMode);
		if (SUCCEEDED(hr))
		{
			CComPtr < IMoniker > spMK;
			hr = ::CreateFileMoniker(pszFileName, &spMK);
			CComPtr < IRunningObjectTable > spROT;
			hr = ::GetRunningObjectTable(NULL, &spROT);
			// mit grfFlags = 0 wird NUR das verhalten bzgl. IExternalConnection gesteuert
			// harte/interne referenzen koennen/duerfen NIEMALS ausser kraft gesetzt werden.
			hr = spROT->Register(0, GetUnknown(), spMK, &m_lCookieRegister);
			if (SUCCEEDED(hr))
			{
				m_strFileName = pszFileName;
				return NOERROR;
			}
		}
	}
	catch (const web::json::json_exception& e)
	{
		e.what(); // json parser failed
	}

	return E_FAIL;
}

STDMETHODIMP CTokenFile::Save(
	/* [unique][in] */ __RPC__in_opt LPCOLESTR pszFileName,
	/* [in] */ BOOL fRemember)
{
	_ASSERT(NULL == pszFileName); // we support ONLY Save
	_ASSERT(!m_strFileName.IsEmpty());

	web::json::value root;
	root[_T("access_token")] = web::json::value(m_strAccessToken);
	root[_T("expires_in")] = web::json::value(86400);
	root[_T("refresh_token")] = web::json::value(m_strRefreshToken);
	root[_T("token_type")] = web::json::value(m_strTokenType);

	std::fstream os(m_strFileName, std::fstream::trunc | std::fstream::out);
	root.serialize(os); // https://casablanca.codeplex.com/wikipage?title=JSON&referringTitle=Documentation
	ATLTRACE2(atlTraceRefcount, 0, _T("CTokenFile(%ls)::IPersistFile::Save()\n"), m_strFileName);
	return NOERROR;
}

// IAuthorize implementation
STDMETHODIMP CTokenFile::AuthorizeRequest(
	/* [in] */ IDispatch* pXMLHttpReq,
	/* [out] */ BSTR* pbstrAccessToken)
{
#ifdef _DEBUG
	// IPersistFile::Load() MUSS (erfolgreich) gelaufen sein
	_ASSERT(!m_strClientId.IsEmpty());

	// es MUSS eine gueltige konfiguration vorliegen
	// wurde durch IPersistFile::Load() geladen
	_ASSERT(!m_strAccessToken.IsEmpty());
#else
	if (NULL == pbstrAccessToken)
		return E_POINTER;
	if (m_strAccessToken.IsEmpty())
		return E_UNEXPECTED;
#endif

	// wenn aktuell ein Renew() am laufen/pending ist kann man JETZT schon sagen das der hier zu authorisierende request fehlschlagen wird!
	// der einzige vorteil das hier NICHT ABZUFANGEN ist das der request dann durch den caller vervollstaendigt und letztlich durch das
	// zwangslaeufig resultierende LockRenew verzoegert wird bis ein gueltiges token vorliegt und dann erneut gestartet wird
	// _ASSERT(NULL == m_spLockForRenew);

	/*
	* fuer den UseCase: Shutdown
	* MUSS prinzipiell JEDER request in einer liste vermerkt werden.
	* erst wenn die liste leer ist sind alle requests bearbeitet und die anwendung kann beendet werden.
	*
	* wir sollten hier das schema/ansatz eines ATL EXE Servers mit seinm MonitorThread nachempfinden.
	* die geniale loesung HIER der MonitorThread ueberwacht den globalen ModuleLock count und ist letztlich fuer das einstellen der AfxPostQuit Message zustaendig
	*/
	/* MULTIMAP_RETRYREQUEST::value_type value(m_strAccessToken, IUnknownPtr(pXMLHttpReq));
	m_mapRetryRequest.insert(MULTIMAP_RETRYREQUEST::value_type(m_strAccessToken, IUnknownPtr(pXMLHttpReq))); */

	MSXML2::IXMLHTTPRequestPtr spXMLHttp(pXMLHttpReq);
	CString strAuthorization;
	strAuthorization.Format(_T("%s %s"), (LPCTSTR)m_strTokenType, (LPCTSTR)m_strAccessToken);
	HRESULT hr = spXMLHttp->setRequestHeader(L"Authorization", (LPCTSTR)strAuthorization);
	ATLTRACE2(atlTraceGeneral, 0, _T("  CTokenFile(%s)::IAuthorize::AuthorizeRequest() Authorization: %s\n"), m_strClientId, strAuthorization);
	ATLTRACE2(atlTraceGeneral, 1, _T("0x%.8x = CTokenFile::IAuthorize::AuthorizeRequest()\n"), hr);
	*pbstrAccessToken = m_strAccessToken.AllocSysString();
	return hr;
}

STDMETHODIMP CTokenFile::CanRetryImmediately(
	/* [in] */ IDispatch* pXMLHttpReq,
	/* [in] */ BSTR bstrAccessToken)
{
	return m_strAccessToken == bstrAccessToken ? S_FALSE : NOERROR;
}

/*
* das wird erst noetig wenn wir den UseCase: Shutdown, Wait for pending requests implementieren
*/
STDMETHODIMP CTokenFile::FinalizeRequest(/* [in] */ IDispatch* pXMLHttpReq)
{
	return E_NOTIMPL;
}

STDMETHODIMP CTokenFile::LockForRenew(
	/* [in] */ IUnknown* pRenewCallback,
	/* [out] */ IUnknown** ppXMLHttpReq,
	/* [out] */ VARIANT *pvarBody)
{
	ATLTRACE2(atlTraceGeneral, 0, _T("CTokenFile(%s)::IAuthorize::LockForRenew()\n"), m_strClientId);
	if (NULL == m_spLockForRenew)
	{
/*
* das obenstehende if (NULL == m_spLockForRenew) ist nur sicher wenn das alles singlethreaded ist
* differences between Msxml2.ServerXMLHTTP and WinHttp.WinHttpRequest?
* - http://stackoverflow.com/questions/1163045/differences-between-msxml2-serverxmlhttp-and-winhttp-winhttprequest
* - bei verwendung von MSXML2::ServerXMLHTTP40 sind die requests NICHT mehr mit Fiddler2 zu tracen
* see also: WTSDisconnected
*/
		// m_spLockForRenew.CreateInstance(__uuidof(MSXML2::XMLHTTP40)); // for use with Fiddler
		// m_spLockForRenew.CreateInstance(__uuidof(MSXML2::ServerXMLHTTP40)); // for use with WTSDisconnected
		m_spLockForRenew.CreateInstance(XMLHTTP_COMPONENT); // reduce dependencies of foreign components
		ATLTRACE2(atlTraceGeneral, 0, _T("  POST: %s\n"), (LPCTSTR)m_strTokenUri);
		m_spLockForRenew->open(L"POST", (LPCTSTR)m_strTokenUri, VARIANT_TRUE); // "https://webapi.teamviewer.com/api/v1/oauth2/token"

		// Header: L"Content-Type"
		m_spLockForRenew->setRequestHeader(L"Content-Type", L"application/x-www-form-urlencoded");

		CComVariant varBody;
		{
			CString postData;
			postData.Format(_T("grant_type=refresh_token&refresh_token=%s&client_id=%s&client_secret=%s"), m_strRefreshToken, m_strClientId, m_strSecret);
			varBody = postData;
		}

			{
				CString strLen;
				strLen.Format(_T("%d"), ::SysStringLen(V_BSTR(&varBody)));
				m_spLockForRenew->setRequestHeader(L"Content-Length", (LPCTSTR)strLen);
			}

			m_spLockForRenew.QueryInterface(IID_PPV_ARGS(ppXMLHttpReq));
			varBody.Detach(pvarBody);
			return E_PENDING; // unser caller MUSS auf OnReadyStateChange warten
	}
	else
	{
#ifdef _DEBUG
		// typischerweise wurde in der ensprechenden spezialisierung das COM_INTERFACE_ENTRY_CHAIN(CCallbackoAuthImpl < >) vergessen!
		CComQIPtr < IRenewCallback > spRenewCallback(pRenewCallback);
		_ASSERT(NULL != spRenewCallback);
#endif

		m_lstRetryRequest.push_back(IUnknownPtr(pRenewCallback));
		// unser caller (CRenewTokenAsync) soll nichts machen. er wird nicht gebraucht UND
		// transparent zerstoert
		return NOERROR;
	}
}

STDMETHODIMP CTokenFile::UnLockFromRenew(void)
{
	ATLTRACE2(atlTraceGeneral, 0, _T("  CTokenFile(%s)::IAuthorize::UnLockFromRenew()\n"), m_strClientId);

	HRESULT hr = E_FAIL;
	if (200 == m_spLockForRenew->status)
	{
		ATLTRACE2(atlTraceGeneral, 0, _T("  CRenewTokenAsync::IXMLDOMDocumentEvents::UnLockFromRenew() succeeded: ready to try again\n"));
		hr = UpdateAccessRefreshToken((BSTR)m_spLockForRenew->responseText);
		hr = NOERROR; // tell your caller: ready to try again
	}

	else if (400 == m_spLockForRenew->status)
	{
		// authorisation failed
		// 400 der zugriff wurde vom user EXPLIZIT gecanceled!
		// wir muessen das "client_secrect_<ClientId>.TokenResponse-user" loeschen. der ConsentWorkflow MUSS erneut durchgefuehrt werden 
		ATLTRACE2(atlTraceRefcount, 0, _T("  CRenewTokenAsync::IXMLDOMDocumentEvents::UnLockFromRenew() User canceled Token! perform ConsentWorkflow again\n"));

		/*
		* anstatt unseren clients, indirekt ueber das loeschen der datei, mit zu teilen das JETZT schluss ist
		* gibt unser Workflow kuenftig bei jeder AuthorizeRequest, CanRetryImmediately,... ein E_UNEXPECTED zurueck
		* das File loeschen wir natuerlich auch so das kuenftige anforderungen auch fehlschlagen
		*
		* hmmm file loeschen, eher nicht so gut bzw. evtl. nicht ausreichend denn das sind ja wir das heisst mindestens:
		* - wir liefern ab jetzt bei JEDEM aufruf ein E_FAIL
		* - durch das loeschen verhindern wir zuverlaessig das jemand einen server mit abgelaufenen credentials erzeugt
		*
		* alternative:
		* - wir schreiben das file mit ""expires_in" : 0" zurueck
		*/

		hr = E_FAIL; // tell your caller: stop immendiately
	}

	else
	{
		ATLTRACE2(atlTraceGeneral, 0, _T("  CRenewTokenAsync::IXMLDOMDocumentEvents::UnLockFromRenew() failed: %d (HTTP STATUS)\n"), m_spLockForRenew->status);
		hr = E_FAIL; // tell your caller: stop immendiately
	}

	// durch finalize aller pending requests wird zumindest der initiale (m_spLockForRenew)
	// hinten angestellt, denn dieser wird ja letztlich erst vom caller durch behandeln des RetC verabeitet. erneuert
	// nachdem aber alle requests async sind darf sich sowieso keiner auf eine reihenfolge verlassen.
#ifdef _DEBUG
	if (SUCCEEDED(hr))
		ATLTRACE2(atlTraceGeneral, 0, _T("  CRenewTokenAsync::IXMLDOMDocumentEvents::UnLockFromRenew() requeue: %d pending requests\n"), m_lstRetryRequest.size());
	else
		ATLTRACE2(atlTraceGeneral, 0, _T("  CRenewTokenAsync::IXMLDOMDocumentEvents::UnLockFromRenew() terminate: %d pending requests\n"), m_lstRetryRequest.size());
#endif
	while (m_lstRetryRequest.size())
	{
		CComQIPtr < IRenewCallback > spRenewCallback(m_lstRetryRequest.front().m_T);
		m_lstRetryRequest.pop_front();
		if (SUCCEEDED(hr))
			spRenewCallback->Continue();
		else
			spRenewCallback->Terminate();
	}

	m_spLockForRenew.Release(); // unlock
	return hr;
}
