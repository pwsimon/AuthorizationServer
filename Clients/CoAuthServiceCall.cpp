// CoAuthServiceCall.cpp : implementation file
//

#include "stdafx.h"
#include "CoAuthServiceCall.h"
#include "CoAuthRenewTokenAsync.h"

// CoAuthServiceCall
IMPLEMENT_DYNCREATE(CoAuthServiceCall, CCmdTarget)

BEGIN_MESSAGE_MAP(CoAuthServiceCall, CCmdTarget)
END_MESSAGE_MAP()

BEGIN_INTERFACE_MAP(CoAuthServiceCall, CCmdTarget)
	INTERFACE_PART(CoAuthServiceCall, __uuidof(oAuthLib::IRenewCallback), RenewCallback)
END_INTERFACE_MAP()

BEGIN_DISPATCH_MAP(CoAuthServiceCall, CCmdTarget)
	DISP_FUNCTION_ID(CoAuthServiceCall, "OnReadyStateChange", 0, ReadyStateChange, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()

// IDispatch implementation
void CoAuthServiceCall::ReadyStateChange()
{
	InternalAddRef(); // lock protect your instance

	switch (m_spRequest->readyState) {
	case READYSTATE_LOADING:
	{
		TRACE2("CoAuthServiceCall(%ls)::IXMLDOMDocumentsEvent::OnReadyStateChange() logical state: %d, m_spRequest->readyState: READYSTATE_LOADING\n", (BSTR)m_bstrUrl, m_eState);

		/*
		* siehe auch: kommentar zu
		*   CoAuthServiceCall::m_bstrAccessToken UND
		*   IWorkflow::AuthorizeRequest
		*/
		oAuthLib::IAuthorizePtr spAuthorize;
		if (SUCCEEDED(GetTokenServer(&spAuthorize)))
			spAuthorize->AuthorizeRequest(m_spRequest, &m_bstrAccessToken);
	}
	break;
	case READYSTATE_COMPLETE:
	{
		TRACE2("CoAuthServiceCall(%ls)::IXMLDOMDocumentsEvent::OnReadyStateChange() logical state: %d, m_spRequest->readyState: READYSTATE_COMPLETE\n", (BSTR)m_bstrUrl, m_eState);

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
/*
* das ist natuerlich LUXUS pur bzw. schon zuviel spezialisierung. es gibt ja auch noch XML
* https://casablanca.codeplex.com/wikipage?title=JSON&referringTitle=Documentation
*
* CAUTION: this is necessary to avoid false memory leaks reported by the MFC framework;
* http://codexpert.ro/blog/2015/05/23/using-lambdas-in-mfc-applications-part-3-dealing-with-c-rest-sdk/
*/
			web::json::value result = web::json::value::parse(utility::string_t(m_spRequest->responseText));
			onSucceeded(result); // wir setzen das onSucceeded NICHT in abhaengigkeit vom m_spRequest->status
#else

			onSucceeded(); // wir setzen das onSucceeded NICHT in abhaengigkeit vom m_spRequest->status
#endif

			// break reference cycle
			// Remove sink from xml http request, hier faellt evtl. die letzte referenz
			// entweder KEINEN code mehr ausfuehren ODER Lock/Unlock
			m_spRequest->put_onreadystatechange(NULL);
		}

		else if (401 == m_spRequest->status && InitialRequest == m_eState)
		{
			oAuthLib::IAuthorizePtr spAuthorize;
			if (SUCCEEDED(GetTokenServer(&spAuthorize)))
			{
				const HRESULT _hr = spAuthorize->raw_CanRetryImmediately(m_spRequest, m_bstrAccessToken);
				_ASSERT(SUCCEEDED(_hr)); // per definition gibt es hier KEIN E_FAIL/E_NOTIMPL ...
				if (S_FALSE == _hr)
				{
					/*
					* der ERSTE request der ein 401 empfaengt bzw. ALLE folge requests, die beendet wurden OHNE das wir bereits ein NEUES GUELTIGES access_token haben, laufen hier rein.
					* werden vom client bzw. den clients die requests schneller abgesetzt als ein ExchangeToken() mit gueltigem access_token zurueckkommt muss der second try gequeued werden.
					*/

					CoAuthRenewTokenAsync* pRenewTokenAsync = DYNAMIC_DOWNCAST(CoAuthRenewTokenAsync, RUNTIME_CLASS(CoAuthRenewTokenAsync)->CreateObject());
					ASSERT(1 == pRenewTokenAsync->m_dwRef);
					// dieses object kuemmert sich transparent und OHNE jegliches zutun des programmieres
					// darum das es einen pThis->onSucceeded() bzw. pThis->onFailed(); gibt
					// diese referenz MUSS VOR dem aufruf von ::Init() gelockt anschliessend freigegeben UND NICHT gespeichert werden!
					oAuthLib::IRenewCallbackPtr spCallback(GetInterface(&__uuidof(oAuthLib::IRenewCallback)));
					pRenewTokenAsync->Init(spAuthorize, spCallback);
					pRenewTokenAsync->ExternalRelease();
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
					oAuthLib::IRenewCallbackPtr spCallback(GetInterface(&__uuidof(oAuthLib::IRenewCallback)));
					spCallback->Continue();
					// here we go: NEUES GUELTIGES access_token
					ASSERT(0 < m_bstrAccessToken.Length());
				}
			}
			else
			{
				// saudummer zustand wir koennen den fehler NICHT an den aufrufer liefern
				// ohne Workflow object geht nix da machen wir ganz ALLGEMEIN schluss
				m_eState = Finish;
				onFailed();
				m_spRequest->put_onreadystatechange(NULL);
			}
		}

		else
		{
			// IRGENDEIN (unbehandelter) http-status
			m_eState = Finish;
			onFailed();

			// break reference cycle
			// Remove sink from xml http request, hie faellt evtl. die letzte referenz
			// entweder KEINEN code mehr ausfuehren ODER Lock/Unlock
			m_spRequest->put_onreadystatechange(NULL);
		}
	}
	break;
	default:
		TRACE2("  logical state: %d, m_spRequest->readyState: %d\n", m_eState, m_spRequest->readyState);
		break;
	}
	InternalRelease(); // unlock your instance
}

CoAuthServiceCall::CoAuthServiceCall()
{
	__super::EnableAutomation();
}

void CoAuthServiceCall::OnFinalRelease()
{
	TRACE0("CoAuthServiceCall::OnFinalRelease()\n");
	CCmdTarget::OnFinalRelease();
}

// C/C++ Interface
HRESULT CoAuthServiceCall::Init(LPCTSTR szMethod, LPCTSTR szUrl)
{
	_ASSERT(NULL == m_spRequest); // initalize only once
	m_spRequest.CreateInstance(__uuidof(MSXML2::XMLHTTP40));

	// add sink to xml http request
	HRESULT hr = m_spRequest->put_onreadystatechange(GetIDispatch(FALSE));
	_ASSERT(SUCCEEDED(hr));

	m_eState = InitialRequest;
	m_bstrMethod = szMethod;
	m_bstrUrl = szUrl;
	TRACE2("  %s: %s (first trial)\n", szMethod, szUrl);

	/*
	* hier laeuft schon der erste (synchrone) Callback (OnReadStateChange(READYSTATE_LOADING))
	* ungluecklicherweise koennen wir fuer den fall das der IWorkflow::AuthorizeRequest() failed KEINEN returnwert/exception liefern
	* ergo wird das IXMLHTTPRequest::send() unbedingt nachgeschoben. das heist wir muessen spaeter evtl. auf den falschen fehler 401 reagieren.
	*/
	m_spRequest->open(m_bstrMethod, m_bstrUrl, VARIANT_TRUE);
	m_spRequest->send();
	return E_PENDING;
}

// IRenewCallback implementation
DELEGATE_IUNK_INTERFACE(CoAuthServiceCall, RenewCallback)
STDMETHODIMP CoAuthServiceCall::XRenewCallback::raw_Continue()
{
	METHOD_PROLOGUE(CoAuthServiceCall, RenewCallback)

	HRESULT hr = E_FAIL;
	pThis->m_eState = RetryOnce;
	try {
		ATLTRACE2(atlTraceGeneral, 0, _T("  %ls: %ls (retry once)\n"), (BSTR)pThis->m_bstrMethod, (BSTR)pThis->m_bstrUrl);
		pThis->m_spRequest->open(pThis->m_bstrMethod, pThis->m_bstrUrl, VARIANT_TRUE); // hier laeuft schon der erste (synchrone) Callback (OnReadStateChange(READYSTATE_LOADING))
		pThis->m_spRequest->send();
		hr = E_PENDING;
	}
	catch (const _com_error& e) {
		hr = e.Error();
	}
	return hr;
}

STDMETHODIMP CoAuthServiceCall::XRenewCallback::raw_Terminate()
{
	METHOD_PROLOGUE(CoAuthServiceCall, RenewCallback)

	pThis->m_eState = Finish;
	return NOERROR;
}
