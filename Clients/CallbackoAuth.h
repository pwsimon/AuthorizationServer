#pragma once

#ifdef _DEBUG
	#include <Wtsapi32.h>
#endif

#define ID_XMLHTTPREQUESTEVENTS  1
#define MSXML_VERSION_MAJOR      4
#define MSXML_VERSION_MINOR      0

static _ATL_FUNC_INFO OnEventInfo = { CC_STDCALL, VT_EMPTY, 0 };

/*
* diese klasse fuehrt einen Renew request, asynchron, OHNE wiederholung aus.
* http://www.codeguru.com/cpp/com-tech/atl/atl/article.php/c3601/Implementing-XMLHTTPRequest-onReadyStateChange-in-C.htm
*/
class ATL_NO_VTABLE CRenewTokenAsync :
	  public CComObjectRootEx < CComSingleThreadModel >
	, public CComCoClass < CRenewTokenAsync >
	, public IPersistImpl < CRenewTokenAsync >
	, public IDispEventSimpleImpl </*nID =*/ ID_XMLHTTPREQUESTEVENTS, CRenewTokenAsync, &__uuidof(MSXML2::XMLDOMDocumentEvents) >
{
public:
	BEGIN_COM_MAP(CRenewTokenAsync)
		COM_INTERFACE_ENTRY(IPersist)
	END_COM_MAP()

	BEGIN_SINK_MAP(CRenewTokenAsync)
		SINK_ENTRY_INFO(/*nID =*/ ID_XMLHTTPREQUESTEVENTS, __uuidof(MSXML2::XMLDOMDocumentEvents), /*dispid =*/ 0, OnReadyStateChange, &OnEventInfo)
	END_SINK_MAP()

	HRESULT FinalConstruct()
	{
		HRESULT hr = NOERROR;
		ATLTRACE2(atlTraceRefcount, 1, _T("0x%.8x = CRenewTokenAsync::FinalConstruct()\n"), hr);
		return hr;
	}

	HRESULT Init(oAuthLib::IAuthorize* pAuthorize, oAuthLib::IRenewCallback* pRenewCallback)
	{
		_ASSERT(NULL == m_spRequest); // initalize only once
		_ASSERT(NULL == m_spRenewCallback);
		_ASSERT(NULL != pRenewCallback);
		if (NULL == pRenewCallback)
			return E_INVALIDARG;

		HRESULT hrRet = E_FAIL;
		try
		{
			m_spAuthorize = pAuthorize;
			IUnknownPtr spReq;
			CComVariant varBody;

			/*
			* per definition IAuthorize::LockForRenew() liefern wir E_PENDING
			* ergo koennen wir das ExceptionHandling fuer den IAuthorize SmartPtr
			* in diesem SPEZIELLEN fall NICHT brauchen
			*/
			const HRESULT _hr = m_spAuthorize->raw_LockForRenew(pRenewCallback, &spReq, &varBody); // wir koennen CTokenFile gleich ein xmlhttpreq object bauen lassen
			if (E_PENDING == _hr)
			{
				m_spRenewCallback = pRenewCallback;
				m_spRequest = spReq;

				// add sink to xml http request
				IDispatchPtr spSink;
				IDispEventSimpleImpl </*nID =*/ ID_XMLHTTPREQUESTEVENTS, CRenewTokenAsync, &__uuidof(MSXML2::XMLDOMDocumentEvents) >::_LocDEQueryInterface(IID_PPV_ARGS(&spSink));
				m_spRequest->put_onreadystatechange(spSink);

				return m_spRequest->send(varBody);
			}

			else if (NOERROR == _hr)
			{
				// nothing to do. no references stored the object will be destroyed as soon as possible
				return NOERROR;
			}

			hrRet = _hr;
		}
		catch (const _com_error& e)
		{
			hrRet = e.Error(); // resource not found, whatever ...
		}

		// es ist was schiefgegangen wir canceln den request manuell/sofort
		pRenewCallback->Terminate();
		return hrRet;
	}

	void FinalRelease()
	{
		ATLTRACE2(atlTraceRefcount, 1, _T("CRenewTokenAsync::FinalRelease()\n"));
	}

	// IXMLDOMDocumentsEvent
	void __stdcall OnReadyStateChange()
	{
		AddRef(); // lock protect your instance
		try
		{
			ATLTRACE2(atlTraceGeneral, 1, _T("CRenewTokenAsync::IXMLDOMDocumentEvents::OnReadyStateChange() readyState: 0x%.8x\n"), m_spRequest->readyState);
			switch (m_spRequest->readyState)
			{
			case READYSTATE_COMPLETE: // = 4
			{
/*
* das hier uebergebene X,Y,Z this hat KEINERLEI funktionale bedeutung und dient lediglich dazu
* dem progrmierer auf evtl. fehler hinzuweisen.
* Hintergrund:
*   es kann NUR EINEN "wirksamen" CRenewTokenAsync geben. Das ist der der als erstes erzeugt wurde.
*   NUR der CRenewTokenAsync der Lock aufgerufen hat kann auch UnLock aufrufen!
*   mit dem ersten CRenewTokenAsync wird the<Service> (IAuthorize) verriegelt.
*   damit folgerequests nicht blockieren und nicht verlorengehen werden sie in einer queue aufbewahrt.
*
*   m_spAuthorize kennt den m_spRequest weil er ihn ja initial selbst erstellt hat.
*   m_spAuthorize braucht den m_spRequest weil dort das ergebnis des "Renew" ermittelt wird.
*   wir (CRenewTokenAsync) brauchen den m_spRequest lediglich um den callback (OnReadyStateChange) zu registrien bzw. zu deregistrieren.
*/
				HRESULT hr = m_spAuthorize->raw_UnLockFromRenew();
				_ASSERT(SUCCEEDED(hr));

				// break reference cycle
				// Remove sink from xml http request
				m_spRequest->put_onreadystatechange(NULL);

				if (SUCCEEDED(hr))
				{
					ATLTRACE2(atlTraceGeneral, 1, _T("  continue previous workflow\n"));
					m_spRenewCallback->Continue();
				}
				else
				{
					ATLTRACE2(atlTraceGeneral, 1, _T("  terminate previous workflow\n"));
					m_spRenewCallback->Terminate();
				}
				m_spRenewCallback = NULL;
			}
			break;

			case READYSTATE_UNINITIALIZED: // = 0,
			case READYSTATE_LOADING: // = 1,
			case READYSTATE_LOADED: // = 2,
			case READYSTATE_INTERACTIVE: // = 3,
			default: break;
			}
		}
		catch (const _com_error& e)
		{
			HRESULT hr = e.Error();
		}

		Release(); // unlock instance, may the last one
	}

private:
	oAuthLib::IAuthorizePtr m_spAuthorize;
	oAuthLib::IRenewCallbackPtr m_spRenewCallback;
	MSXML2::IXMLHTTPRequestPtr m_spRequest;
};

template < class T >
class ATL_NO_VTABLE CCallbackoAuthImpl :
	  public oAuthLib::IRenewCallback
	, public IDispEventSimpleImpl </*nID =*/ ID_XMLHTTPREQUESTEVENTS, CCallbackoAuthImpl < T >, &__uuidof(MSXML2::XMLDOMDocumentEvents) >
{
public:
	BEGIN_COM_MAP(CCallbackoAuthImpl < T >)
		COM_INTERFACE_ENTRY(oAuthLib::IRenewCallback)
	END_COM_MAP()

	BEGIN_SINK_MAP(CCallbackoAuthImpl < T >)
		SINK_ENTRY_INFO(/*nID =*/ ID_XMLHTTPREQUESTEVENTS, __uuidof(MSXML2::XMLDOMDocumentEvents), /*dispid =*/ 0, OnReadyStateChange, &OnEventInfo)
	END_SINK_MAP()

/*
* C/C++ interface
*   used by our creator/client
*
* Hinweise:
* Methoden aus dem C/C++ interface sind im gegensatz zu COM interface "relativ" frei in wahl ob sie exception werfen oder nicht
*/
	HRESULT Init(LPCTSTR szMethod, LPCTSTR szUrl) {
		T* pThis = static_cast<T*>(this);
		_ASSERT(NULL == m_spRequest); // initalize only once
		m_spRequest.CreateInstance(XMLHTTP_COMPONENT);

		// add sink to xml http request
		IDispatchPtr spSink;
		HRESULT hr = IDispEventSimpleImpl <ID_XMLHTTPREQUESTEVENTS, CCallbackoAuthImpl < T >, &__uuidof(MSXML2::XMLDOMDocumentEvents) >::_LocDEQueryInterface(IID_PPV_ARGS(&spSink));
		hr = m_spRequest->put_onreadystatechange(spSink);
		_ASSERT(SUCCEEDED(hr));

		m_eState = InitialRequest;
		m_bstrMethod = szMethod;
		m_bstrUrl = szUrl;
		ATLTRACE2(atlTraceGeneral, 0, _T("  %ls: %ls (first trial)\n"), (BSTR)m_bstrMethod, (BSTR)m_bstrUrl);

/*
* hier laeuft schon der erste (synchrone) Callback (OnReadStateChange(READYSTATE_LOADING))
*/
		CComVariant varAsync(VARIANT_TRUE);
		m_spRequest->open(m_bstrMethod, m_bstrUrl, varAsync);

/*
* ungluecklicherweise koennen wir fuer den fall das der IWorkflow::AuthorizeRequest() failed KEINEN returnwert/exception liefern
* ergo wird das IXMLHTTPRequest::send() unbedingt nachgeschoben.
* das heist wir muessen spaeter evtl. auf die falschen/folge fehler reagieren.
* z.B. 401 Unauthorized oder 500 InvalidRequest, ...
*
* gluecklicherweise muessen wir uns das verwendete m_bstrAccessToken merken ...
* nachdem OHNE access_token nix geht ist das ein sicheres zeichen fuer einen fehler
* die pruefung bzw. der vorzeitige exit hat keinen einfluss auf die funktionalitaet
* vereinfacht aber vermutlich die fehlersuche/analyse da man ansonsten von einer fehlerhaften Authentication ausgeht
* in diesem fall wurde ueberhaupt KEIN access_token angehaengt
*/
		if (0 == m_bstrAccessToken.Length())
		{
			ATLTRACE2(atlTraceGeneral, 0, _T("  no access_token from AuthenticationServer\n"));
			m_eState = Finish;
			pThis->onFailed();
			m_spRequest->put_onreadystatechange(NULL);
			return E_FAIL;
		}

#ifdef AUTHORIZATION_SERVER_SUPPORT_JSON
		m_spRequest->setRequestHeader(L"Accept", L"application/json");
		// m_spRequest->setRequestHeader(L"Accept", L"application/json,application/xml");
#else

		m_spRequest->setRequestHeader(L"Accept", L"application/xml");
#endif
		m_spRequest->send();
		return VARIANT_TRUE == V_BOOL(&varAsync) ? E_PENDING : NOERROR;
	}

/*
* IRenewCallback implementation
*   used by our async sub task
*
* Hinweise:
*   NEVER throw exception across COM boundary
*/
	STDMETHOD(raw_Continue)()
	{
		T* pThis = static_cast<T*>(this);

		HRESULT hr = E_FAIL;
		m_eState = RetryOnce;
		try {
			ATLTRACE2(atlTraceGeneral, 0, _T("  %ls: %ls (retry once)\n"), (BSTR)m_bstrMethod, (BSTR)m_bstrUrl);
			m_spRequest->open(m_bstrMethod, m_bstrUrl, VARIANT_TRUE); // hier laeuft schon der erste (synchrone) Callback (OnReadStateChange(READYSTATE_LOADING))
#ifdef AUTHORIZATION_SERVER_SUPPORT_JSON
			m_spRequest->setRequestHeader(L"Accept", L"application/json");
			// m_spRequest->setRequestHeader(L"Accept", L"application/json,application/xml");
#else

			m_spRequest->setRequestHeader(L"Accept", L"application/xml");
#endif
			m_spRequest->send();
			hr = E_PENDING;
		}
		catch (const _com_error& e) {
			hr = e.Error();
		}
		return hr;
	}

	STDMETHOD(raw_Terminate)()
	{
		T* pThis = static_cast<T*>(this);

		m_eState = Finish;
		return NOERROR;
	}

	// IXMLDOMDocumentsEvent
	void __stdcall OnReadyStateChange() {
		T* pThis = static_cast<T*>(this);
		USES_CONVERSION;
		AddRef(); // lock protect your instance

		switch (m_spRequest->readyState) {
			case READYSTATE_LOADING:
				{
					ATLTRACE2(atlTraceGeneral, 1, _T("CCallbackoAuthImpl<T>(%ls)::IXMLDOMDocumentsEvent::OnReadyStateChange() logical state: %d, m_spRequest->readyState: READYSTATE_LOADING\n"), (BSTR)pThis->m_bstrUrl, m_eState);

/*
* siehe auch: kommentar zu
*   CCallbackoAuthImpl<T>::m_bstrAccessToken UND
*   IWorkflow::AuthorizeRequest
*/
					oAuthLib::IAuthorizePtr spAuthorize;
					if (SUCCEEDED(pThis->GetTokenServer(&spAuthorize)))
						spAuthorize->AuthorizeRequest(m_spRequest, &m_bstrAccessToken);
				}
				break;
			case READYSTATE_COMPLETE:
				{
					ATLTRACE2(atlTraceGeneral, 1, _T("CCallbackoAuthImpl<T>(%ls)::IXMLDOMDocumentsEvent::OnReadyStateChange() logical state: %d, m_spRequest->readyState: READYSTATE_COMPLETE, HTTP-Status: %d\n"), (BSTR)pThis->m_bstrUrl, m_eState, m_spRequest->status);

#ifdef _DEBUG
/*
* WTSDisconnected (4) => WinStation logged on without client -> HTTP-Status: 0
* - JEDER request mittels XMLHTTPRequest failed solange ich KEINE desktop session habe!
*   das ist offensichtlich KEIN direkter zusammenhang: also nach einem "connect" ist NICHT garantiert das der naechste request durchgeht ABER:
*   nach ein paar sekunden faengt sich das system wieder von alleine???
* - der request geht definitiv raus. d.H. im server-log sehe ich jeden request
*   nur kommt der vom server gelieferte HTTP-Status hier im client nie an???
* siehe auch: IRGENDEIN (unbehandelter) HTTP-Status
*/
					WTS_CONNECTSTATE_CLASS* pResult = NULL;
					DWORD dwBytesReturned = 0;
					if (::WTSQuerySessionInformation(
						WTS_CURRENT_SERVER_HANDLE,
						WTS_CURRENT_SESSION,
						WTSConnectState,
						(LPTSTR*)&pResult,
						&dwBytesReturned))
					{
						ATLTRACE2(atlTraceGeneral, 1, _T("  READYSTATE_COMPLETE, WTSConnectState: %d, HTTP-Status: %d\n"), *pResult, m_spRequest->status);
						::WTSFreeMemory(pResult);
					}
#endif

/*
* das wird erst noetig wenn wir den UseCase: Shutdown, Wait for pending requests implementieren
* CComPtr < IWorkflow > spWorkflow;
* pThis->GetWorkflow(&spWorkflow);
* spWorkflow->FinalizeRequest(m_spRequest);
*/

					if (200 == m_spRequest->status)
					{
						m_eState = Finish;

#ifdef AUTHORIZATION_SERVER_SUPPORT_JSON
						// das ist natuerlich LUXUS pur bzw. schon zuviel spezialisierung. es gibt ja auch noch XML
						// https://casablanca.codeplex.com/wikipage?title=JSON&referringTitle=Documentation
						web::json::value result = web::json::value::parse(utility::string_t(m_spRequest->responseText));
						pThis->onSucceeded(result); // wir setzen das onSucceeded NICHT in abhaengigkeit vom m_spRequest->status
#else
						pThis->onSucceeded();
#endif

						// break reference cycle
						// Remove sink from xml http request, hier faellt evtl. die letzte referenz
						// entweder KEINEN code mehr ausfuehren ODER Lock/Unlock
						m_spRequest->put_onreadystatechange(NULL);
					}

					else if (401 == m_spRequest->status && InitialRequest == m_eState)
					{
						oAuthLib::IAuthorizePtr spAuthorize;
						if (SUCCEEDED(pThis->GetTokenServer(&spAuthorize)))
						{
							const HRESULT _hr = spAuthorize->raw_CanRetryImmediately(m_spRequest, pThis->m_bstrAccessToken);
							_ASSERT(SUCCEEDED(_hr)); // per definition gibt es hier KEIN E_FAIL/E_NOTIMPL ...
							if (S_FALSE == _hr)
							{
								/*
								* der ERSTE request der ein 401 empfaengt bzw. ALLE folge requests, die beendet wurden OHNE das wir bereits ein NEUES GUELTIGES access_token haben, laufen hier rein.
								* werden vom client bzw. den clients die requests schneller abgesetzt als ein ExchangeToken() mit gueltigem access_token zurueckkommt muss der second try gequeued werden.
								*/
								CComObject < CRenewTokenAsync >* pRenewTokenAsync = NULL;
								CComObject < CRenewTokenAsync >::CreateInstance(&pRenewTokenAsync);
								// dieses object kuemmert sich transparent und OHNE jegliches zutun des programmieres
								// darum das es einen pThis->onSucceeded() bzw. pThis->onFailed(); gibt
								// diese referenz MUSS VOR dem aufruf von ::Init() gelockt anschliessend freigegeben UND NICHT gespeichert werden!
								IUnknownPtr spLock(pRenewTokenAsync->GetUnknown());
								pRenewTokenAsync->Init(spAuthorize, pThis);
							}
							else
							{
								/*
								* wir haben bereits ein NEUES GUELTIGES access_token empfangen.
								* wir koennen diesen request SOFORT neu anstossen. MUSS NATUERLICH ASYNC sein.
								*/
								// wir loeschen das alte/unbrauchbare access_token aus dem initialen request
								m_bstrAccessToken.Empty();
								// mit IRenewCallback::Continue() wird der m_spRequest NEU initialisiert und indirekt ueber IWorkflow::AuthorizeRequest() wieder authorisiert.
								pThis->Continue();
								// here we go: NEUES GUELTIGES access_token
								_ASSERT(0 < m_bstrAccessToken.Length());
							}
						}
						else
						{
							// saudummer zustand wir koennen den fehler NICHT an den aufrufer liefern
							// ohne Workflow object geht nix da machen wir ganz ALLGEMEIN schluss
							m_eState = Finish;
							pThis->onFailed();
							m_spRequest->put_onreadystatechange(NULL);
						}
					}

					else
					{
/*
* IRGENDEIN (unbehandelter) HTTP-Status
* spezialfaelle:
* - HTTP-Status code: 0 kann reproduziert werden wenn ich mich von der WTS-Session detache???
*   what does this mean in MS XMLHTTP?, http://stackoverflow.com/questions/872206/http-status-code-0-what-does-this-mean-in-ms-xmlhttp
*/
						m_eState = Finish;
						pThis->onFailed();

						// break reference cycle
						// Remove sink from xml http request, hier faellt evtl. die letzte referenz
						// entweder KEINEN code mehr ausfuehren ODER Lock/Unlock
						m_spRequest->put_onreadystatechange(NULL);
					}
				}
				break;
			default:
				ATLTRACE2(atlTraceGeneral, 1, _T("  logical state: %d, m_spRequest->readyState: %d\n"), m_eState, m_spRequest->readyState);
				break;
		}
		Release(); // unlock your instance
	};

protected:
	MSXML2::IXMLHTTPRequestPtr m_spRequest;

private:
	enum { InitialRequest, RetryOnce, Finish } m_eState;
	_bstr_t m_bstrMethod;
	_bstr_t m_bstrUrl;

	// wir MUESSEN uns das zuletzt benutzte "access_token" merken
	// wir entscheiden zum ZEITPUNKT wo dieser request mit einem 401 zurueckkommt ob:
	// a.) wir die ersten sind die erkennen das ein Exchange noetig ist (und den Exchange triggern)
	// b.) aktuell bereits ein Exchange am laufen ist aber noch nicht beendet wurde. (wir wiederholen mit dem neuen token sobald es vorliegt)
	// c.) das access/refresh-token bereits aktualisiert wurde. (und wir direkt wiederholen koennen)
	CComBSTR m_bstrAccessToken;
};
