#include "..\CallbackoAuth.h"

typedef IWorkflowWebImpl < void > CSimulatorWf;

class ATL_NO_VTABLE CSimulatorPing :
	public CComObjectRootEx < CComSingleThreadModel >
	, public CComCoClass < CSimulatorPing >
	, public IPersistFile
	, public CCallbackoAuthImpl < CSimulatorPing >
{
public:
	BEGIN_COM_MAP(CSimulatorPing)
		COM_INTERFACE_ENTRY(IPersist)
		COM_INTERFACE_ENTRY(IPersistFile)
		COM_INTERFACE_ENTRY_CHAIN(CCallbackoAuthImpl < CSimulatorPing >)
	END_COM_MAP()

	HRESULT FinalConstruct()
	{
		ATLTRACE2(atlTraceRefcount, 0, _T("CSimulatorPing::FinalConstruct()\n"));
		m_hrLoadConfig = E_PENDING;

		/*
		* die folgenden member werden durch IPersistFile::Load() aus der config geladen
		* 1.) eine hard-coded initialisierung macht vermutlich nur fuer den fall sinn das KEIN configfile auf der commandline uebergeben wurde
		* 2.) eine hard-coded initialisierung macht vermutlich erst sinn wenn im IPersistFile::Load() festgestellt wurde das die configfile ungueltig war/ist
		*     aktuell wird IPersistFile::Load() nur garantiert wenn eine commandline uebergeben wurde
		*     was machen wir bei einem klassischen start per doppelclick???
		*/
		m_strClientId = _T("SimulatorClientId");
		m_strUrl = _T("http://simulatorauthserver-1310.appspot.com/ping");
		return NOERROR;
	}

	void FinalRelease()
	{
		ATLTRACE2(atlTraceRefcount, 0, _T("CSimulatorPing::FinalRelease()\n"));

#ifdef FEATURE_TASKSCHD
		/*
		* wenn die anwendung fuer "BatchMode" compiliert wurde
		* soll sie mit dem finish des ersten request, egal ob succeeded/failed, terminieren
		*/
		if(SUCCEEDED(m_hrLoadConfig))
			::PostMessage(m_hwndResult, WM_CLOSE, 0, 0);
#else
#endif
	}

	// C/C++ interface
	HRESULT Init(HWND hwndResult)
	{
		m_hwndResult = hwndResult;
#ifdef MONITOR_PENDING_REQUESTS
		::SendMessage(m_hwndResult, WM_USER + 0x10, 2, (LPARAM) this);
#endif

		// base class, dynamic result
		// der HTTP Request liefert je nach "Bearer: <token>" ein 200 oder im fall von expired ein 401
		const HRESULT hr = CCallbackoAuthImpl < CSimulatorPing >::Init(_T("GET"), m_strUrl);
		/*
		* das Init kann schon "synchron" einen fehler liefern.
		* typischerweise fehlendes bzw. defektes konfigurationsfile fuer den service ODER
		* die installation an sich ist defekt z.B. wenn der AuthorisationServer ist nicht registriert/abgestuerzt/...
		* wir legen fest das, fuer eine ZENTRALE fehlerbehandlung, in diesem fall zusaetzlich ein onFailed() ausgeloest wird!
		*/
		return hr;
	}

	HRESULT GetTokenServer(oAuthLib::IAuthorize** ppAuthorize) {
		const CString strMonikerName = CSimulatorWf::FileMonikerDN4TokenResponse((LPCTSTR)m_strClientId);
		return CSimulatorWf::GetTokenServerByDisplayName(strMonikerName, ppAuthorize);
	}

#ifdef MONITOR_PENDING_REQUESTS
	HRESULT Abort() {
		return m_spRequest->abort();
	}
#endif

#ifdef AUTHORIZATION_SERVER_SUPPORT_JSON
	void onSucceeded(const Json::Value& jsonRepCon) {
		ATLTRACE2(atlTraceGeneral, 0, _T("CSimulatorPing::onSucceeded()\n"));
	}
#else
	void onSucceeded() {
#ifdef MONITOR_PENDING_REQUESTS
		::SendMessage(m_hwndResult, WM_USER + 0x10, 0, (LPARAM) this);
#endif

		/*
		* die pruefung auf HTTP-Status == 200 (OK )ist die bedingung das hier ueberhaupt onSucceeded() aufgerufen wird
		* TracePointDef: CSimulatorPing::onSucceeded() HTTP Status: {m_spRequest->status}, {m_spRequest->statusText}
		* Labels/Keywords: 
		*/
		ATLTRACE2(atlTraceGeneral, 0, _T("CSimulatorPing::onSucceeded() HTTP Status: 0x%d, %ls\n"), m_spRequest->status, (BSTR)m_spRequest->statusText);

		MSXML2::IXMLDOMDocument2Ptr spXML(m_spRequest->responseXML);
		ATLTRACE2(atlTraceGeneral, 1, _T("CSimulatorPing::onSucceeded() Result: %ls\n"), (BSTR)spXML->documentElement->xml);
	}
#endif

	void onFailed() {
		try
		{
#ifdef MONITOR_PENDING_REQUESTS
			::SendMessage(m_hwndResult, WM_USER + 0x10, 1, (LPARAM) this);
#endif

/*
* wir muessten hier unterscheiden ob
* a.) eine automatische wiederholung sinn macht z.B. Server nicht erreichbar 50x oder
* b.) KEINEN sinn macht z.B. ungueltiges RefreshToken
*     diesen fall koenen wir aktuell NICHT erkennen denn das m_spRequest ist noch im status 401 AccessDenied
*     somit koennen wir ein einfaches AccessDenied auf fachlicher ebene NICHT von einem ungueltigen RefreshToken unterscheiden
* c.) wir kommen hier, wegen dem zentralen fehlerhandling, auch rein wenn das Init() fehlschlaegt
*/
			switch (getState())
			{
				case CCallbackoAuthImpl::InitialRequest:
					// hier macht das auswerten von m_spRequest noch KEINEN sinn. der request wurde gar nicht abgesetzt weil er NICHT authorized wurde.
					ATLTRACE2(atlTraceGeneral, 0, _T("CSimulatorPing::onFailed() while CCallbackoAuthImpl::InitialRequest, probably missing/invalid token file\n"));
					break;
				case CCallbackoAuthImpl::RetryOnce:
					ATLTRACE2(atlTraceGeneral, 0, _T("CSimulatorPing::onFailed() while CCallbackoAuthImpl::RetryOnce, renew failed, probably invalid refreshtoken\n"));
					break;
				case CCallbackoAuthImpl::Finish:
					ATLTRACE2(atlTraceGeneral, 0, _T("CSimulatorPing::onFailed() while CCallbackoAuthImpl::Finish, more information with m_spRequest->status, ...\n"));
					break;
				default:
					ATLTRACE2(atlTraceGeneral, 0, _T("CSimulatorPing::onFailed() 0x%d, unknown state\n"), getState());
					_ASSERT(FALSE); // unknown state
					break;
			}

			if (READYSTATE_COMPLETE == m_spRequest->readyState)
			{
/*
* die ausgabe von HTTP-Status, statusText, ... ist im fehlerfall wichtiger/sinvoller als im gut fall
*
* eine granulare fehlerauswertung ist hier sicher sinnvoll um folgefehler zu vermeiden
* z.B. wenn der Service voruebergehend nicht erreichbar ist macht es sinn es spaeter nochmal zu versuchen
* bei einer fehlerhaften konfiguration koennen wir das vergessen (die wird sich nicht selbst heilen)
*/
				ATLTRACE2(atlTraceGeneral, 0, _T("CSimulatorPing::onFailed() HTTP Status: 0x%d, %ls\n"), m_spRequest->status, (BSTR)m_spRequest->statusText);
			}
			else
			{
/*
* onFailed() wird auch aufgerufen wenn der AuthenticationServer in einen fehler gelaufen ist.
* z.B. config-files fehlen oder fehlende runtime/installation.
* in diesem fall ist der m_spRequest->readyState noch READYSTATE_LOADING, wurde also niemals ausgefuehrt (send)
* ein zugriff auf m_spRequest->status fuehrt dann zu einer exception
*/
				_ASSERT(READYSTATE_LOADING == m_spRequest->readyState);
			}
		}
		catch (const _com_error& e)
		{
			_ASSERT(FALSE);
		}
	}

	// IPersist Interface
	STDMETHOD(GetClassID)(/* [out] */ __RPC__out CLSID* pClassID)
	{
		*pClassID = CSimulatorPing::GetObjectCLSID();
		return S_OK;
	}

	// IPersistFile
	STDMETHOD(IsDirty)(void) { return E_NOTIMPL; }
	STDMETHOD(Load)(/* [in] */ __RPC__in LPCOLESTR pszFileName, /* [in] */ DWORD dwMode)
	{
		m_hrLoadConfig = E_FAIL;
		/*
		* %appdata%\WTLApplication.xml
		* <root>
		*   <ClientId></ClientId>
		*   <url></url>
		*/
		try
		{
			MSXML2::IXMLDOMDocument2Ptr spDoc(__uuidof(MSXML2::DOMDocument60));
			if (VARIANT_TRUE == spDoc->load(pszFileName))
			{
				m_strClientId = (LPCTSTR)MSXML2::IXMLDOMNodePtr(spDoc->documentElement)->selectSingleNode(_bstr_t(L"ClientId"))->text;
				m_strUrl = (LPCTSTR)MSXML2::IXMLDOMNodePtr(spDoc->documentElement)->selectSingleNode(_bstr_t(L"url"))->text;
				m_hrLoadConfig = NOERROR;
				return NOERROR;
			}
		}
		catch (const _com_error& e)
		{
			m_strClientId.Empty();
			m_strUrl.Empty();
			ATLTRACE2(atlTraceGeneral, 0, _T("CSimulatorPing(unknown)::IPersistFile::Load() FAILED invalid or not found\n"));
		}

		return m_hrLoadConfig;
	}
	STDMETHOD(Save)(/* [unique][in] */ __RPC__in_opt LPCOLESTR pszFileName, /* [in] */ BOOL fRemember) { return E_NOTIMPL; }
	STDMETHOD(SaveCompleted)(/* [unique][in] */ __RPC__in_opt LPCOLESTR pszFileName) { return E_NOTIMPL; }
	STDMETHOD(GetCurFile)(/* [out] */ __RPC__deref_out_opt LPOLESTR *ppszFileName) { return E_NOTIMPL; }

private:
	HWND m_hwndResult;
	CString m_strClientId;
	CString m_strUrl;
	HRESULT m_hrLoadConfig;
};
