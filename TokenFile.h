// TokenFile.h : Declaration of the CTokenFile

#pragma once
#include "resource.h"       // main symbols
#include "oAuth_i.h"

// C++ REST SDK (Codename "Casablanca")
//   NuGet Package, Version 2.7.0 (desktop), https://github.com/Microsoft/cpprestsdk
#include <cpprest/json.h>

#define CLIENT_SECRET			_T("_client_secret.json")

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;

/*
* es MUSS eine ZENTRALE stelle bzw. einen synchronisierten zugriff auf access und refresh-token geben.
* ansonsten koennte es bei nebenlaeufigem zugiff dazu kommen das ZWEI instanzen das, abgelaufene, access_token lesen
* daraufhin werden beide ein renew ausfuehren. jetzt muss EINER der beiden renew requests fehlschlagen (logisch)
* die instance mit dem fehlgeschlgen request wird nun das configfile loeschen. letztlich haben sich die beiden gegenseitig rausgekickt!
*
* diese implementierung MUSS als zentraler eigenstaendiger EXE Singleton vorliegen!
* Begruendung: (Teil 1)
*   mit einer IExternalConnection implementierung bleibt das cookie und der ganze registrierungs/lebensdauer klimbim lokal in dieser klasse
*   jeder client ruft ::CoGetObject() wenn er das TokenFile/Workflow braucht und release wenn nicht mehr. EINFACH FERTIG!
*
* Begruendung: (Teil 2)
*   ein "forced" shutdown des aktiven host prozesses (ECtiClient) MUESSTE einen sicheren/natlosen uebergang auf einen anderen host prozesses garantieren.
*   dazu ist minimum ein Lock des Shutdown waehrend (NULL != m_spLockForRenew) zu garantieren.
*   das wiederum heisst DIESES object kontrolliert seinen host.
*   die komplexitaet ueberblicke ich aktuell nicht bzw. vermeide ich in JEDEM fall.
*
* Begruendung: (Teil 3)
* - das object MUSS sich in der ROT Registrieren und erhaelt ein cookie
*   weiterhin ist das object jetzt von der ROT referenziert und ist damit in der verpflichtung sich aktiv zu deregistrieren.
*   wuerde das object als InProcServer vorliegen so muessen wir beim User initierten shutdown folgende Usecases handlen:
*   a.) WIR haben die aktuelle instanz erzeugt und sind also ursaechlich fuer register/unregister verantwortlich
*   b.) WIR paritipieren an einer bereits bestehen instanz und duerfen in KEINEM fall ein register/unregister ausfuehren.
*   das problem ist das wir direkt von ::CoGetObject() KEINE informationen darueber erhalten ob es sich um fall a.) oder b.) handelt
*   wir koennen als gar nicht sagen ob wir gem. a.) oder b.) handeln muessen
*   Hinweis:
*     technisch koennte man das evtl. dadurch umgehen indem man nach der rueckkehr von ::CoGetObject() den BindContext untersucht.
*     entweder finden sich bereits alle information die einen rueckschluss zulassen ODER wir schreiben sie explizit hinein. ABER
*     das klingt alles andere als EINFACH
*
* - muss ich jeweils einen EIGENEN exe server aufsetzten oder macht es sinn das indirekt als COM-Surrogat zu konfigurieren?
*   - wo sind die unterschiede zwischen EIGENEN exe UND
*   - COM-Surrogat
*     wir starten als DLL diese laesst sich dann 
*/

/*
* in dem moment wo ich das FileMoniker/::CoGetObject gespann/feature nutzen will stellt sich die frage?
* - brauch ich jetzt fuer JEDEN service einen EIGENEN dateitype/class (CSimulatorTokenFile / CSimulatorWf) ODER
*   solange ich KEINE funktionen aus IWorkflowWebImpl < CSimulatorWf > wie pThis->GetAutenticationURI() rufe
* - geht ein generischer type (TokenFile)
*/
class ATL_NO_VTABLE CTokenFile :
	  public CComObjectRootEx<CComSingleThreadModel>
	, public CComCoClass < CTokenFile, &CLSID_TokenFile >
	, public IExternalConnectionImpl < CTokenFile >
	, public IPersistImpl < CTokenFile >
	, public IPersistFile
	, public IAuthorize
{
public:
	DECLARE_REGISTRY_RESOURCEID(IDR_TOKENFILE)

	BEGIN_COM_MAP(CTokenFile)
		COM_INTERFACE_ENTRY(IExternalConnection)
		COM_INTERFACE_ENTRY(IPersistFile) // http://www.bing.com/search?q=filemoniker+ipersistfile&src=IE-TopResult&FORM=IETR02&conversationid=
		COM_INTERFACE_ENTRY(IAuthorize)
	END_COM_MAP()

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct();
	HRESULT DeleteFile();
	HRESULT LoadTokenResponseFromFile(LPCOLESTR pszFileName, DWORD dwMode);
	HRESULT UpdateAccessRefreshToken(BSTR bstrResponseText);

public:
	// IExternalConnectionImpl overrides
	void OnAddConnection(bool bThisIsFirstLock)
	{
		ATLTRACE2(atlTraceRefcount, 4, _T("CTokenFile::OnAddConnection()\n"));
	}

	void OnReleaseConnection(bool bThisIsLastUnlock, bool bLastUnlockReleases)
	{
		ATLTRACE2(atlTraceRefcount, 2, _T("CTokenFile::OnReleaseConnection() bThisIsLastUnlock = %#.8x, bLastUnlockReleases = %#.8x\n"), bThisIsLastUnlock, bLastUnlockReleases);
		if (bThisIsLastUnlock)
		{
			CComPtr < IRunningObjectTable > spROT;
			HRESULT hr = ::GetRunningObjectTable(NULL, &spROT);
			hr = spROT->Revoke(m_lCookieRegister);
			m_lCookieRegister = 0;
		}
	}

	// IPersist Interface
	STDMETHOD(GetClassID)(
		/* [out] */ __RPC__out CLSID* pClassID)
	{
		return IPersistImpl < CTokenFile >::GetClassID(pClassID);
	}

	// IPersistFile Interface
	// siehe: TokeFile.rgs
	//   .TokenResponse-user = s 'oAuth.TokenFile.1'
	// IMoniker::BindToObject method
	//   https://msdn.microsoft.com/en-us/library/windows/desktop/ms691433(v=vs.85).aspx
	STDMETHOD(IsDirty)(void) { return E_NOTIMPL;  }
	STDMETHOD(Load)(/* [in] */ __RPC__in LPCOLESTR pszFileName, /* [in] */ DWORD dwMode);
	STDMETHOD(Save)(/* [unique][in] */ __RPC__in_opt LPCOLESTR pszFileName, /* [in] */ BOOL fRemember);
	STDMETHOD(SaveCompleted)(
		/* [unique][in] */ __RPC__in_opt LPCOLESTR pszFileName)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(GetCurFile)(
		/* [out] */ __RPC__deref_out_opt LPOLESTR *ppszFileName)
	{
		return E_NOTIMPL;
	}

	// IAuthorize
	STDMETHOD(AuthorizeRequest)(/* [in] */ IDispatch* pXMLHttpReq, /* [out] */ BSTR* pbstrAccessToken);
	STDMETHOD(CanRetryImmediately)(/* [in] */ IDispatch* pXMLHttpReq, /* [in] */ BSTR bstrAccessToken);
	STDMETHOD(FinalizeRequest)(/* [in] */ IDispatch* pXMLHttpReq);
	STDMETHOD(LockForRenew)(/* [in] */ IUnknown* pRenewCallback, /* [out] */ IUnknown** ppXMLHttpReq, /* [out] */ VARIANT *pvarBody);
	STDMETHOD(UnLockFromRenew)(void);

private:
	// theFile
	CString m_strFileName;
	DWORD m_lCookieRegister;
	ATL::CString m_strAPIPrefix;

	// content from <APIPrefix>_client_secret.json file
	ATL::CString m_strClientId;
	ATL::CString m_strSecret;

	// durch diese diese information ist diese implementierung vom der API/Hersteller unabhaengig
	ATL::CString m_strAuthUri;
	ATL::CString m_strTokenUri;

	// das ist der kern eines TokenFile (.TokenResponse-user)
	ATL::CString m_strAccessToken;
	ATL::CString m_strRefreshToken;
	ATL::CString m_strTokenType;

	// protect/lock instance and keep async by defer requests
	MSXML2::IXMLHTTPRequestPtr m_spLockForRenew;
	std::list < ATL::CAdapt < IUnknownPtr > > m_lstRetryRequest;
};

OBJECT_ENTRY_AUTO(__uuidof(TokenFile), CTokenFile)
