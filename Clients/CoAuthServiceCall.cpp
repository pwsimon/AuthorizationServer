// CoAuthServiceCall.cpp : implementation file
//

#include "stdafx.h"
#include "CoAuthServiceCall.h"
#include "CoAuthRenewTokenAsync.h"

// CoAuthServiceCall
IMPLEMENT_DYNCREATE(CoAuthServiceCall, CCmdTarget)

BEGIN_MESSAGE_MAP(CoAuthServiceCall, CCmdTarget)
END_MESSAGE_MAP()

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
		TRACE2("CCallbackoAuthImpl<T>(%ls)::IXMLDOMDocumentsEvent::OnReadyStateChange() logical state: %d, m_spRequest->readyState: READYSTATE_LOADING\n", (BSTR)m_bstrUrl, m_eState);

		/*
		* siehe auch: kommentar zu
		*   CCallbackoAuthImpl<T>::m_bstrAccessToken UND
		*   IWorkflow::AuthorizeRequest
		*/
		oAuthLib::IAuthorizePtr spAuthorize;
		if (SUCCEEDED(GetTokenServer(&spAuthorize)))
			spAuthorize->AuthorizeRequest(m_spRequest, &m_bstrAccessToken);
	}
	break;
	case READYSTATE_COMPLETE:
	{
		TRACE2("CCallbackoAuthImpl<T>(%ls)::IXMLDOMDocumentsEvent::OnReadyStateChange() logical state: %d, m_spRequest->readyState: READYSTATE_COMPLETE\n", (BSTR)m_bstrUrl, m_eState);

		/*
		* das wird erst noetig wenn wir den UseCase: Shutdown, Wait for pending requests implementieren
		* CComPtr < IWorkflow > spWorkflow;
		* pThis->GetWorkflow(&spWorkflow);
		* spWorkflow->FinalizeRequest(m_spRequest);
		*/

		if (200 == m_spRequest->status)
		{
			m_eState = Finish;

			// das ist natuerlich LUXUS pur bzw. schon zuviel spezialisierung. es gibt ja auch noch XML
			// https://casablanca.codeplex.com/wikipage?title=JSON&referringTitle=Documentation
			web::json::value result = web::json::value::parse(utility::string_t(m_spRequest->responseText));
			onSucceeded(result); // wir setzen das onSucceeded NICHT in abhaengigkeit vom m_spRequest->status

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
					// dieses object kuemmert sich transparent und OHNE jegliches zutun des programmieres
					// darum das es einen pThis->onSucceeded() bzw. pThis->onFailed(); gibt
					// diese referenz MUSS VOR dem aufruf von ::Init() gelockt anschliessend freigegeben UND NICHT gespeichert werden!
					IUnknownPtr spLock(pRenewTokenAsync->GetInterface(&__uuidof(IUnknown)));
					// pRenewTokenAsync->Init(spAuthorize, pThis);
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
					Continue();
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

CoAuthServiceCall::~CoAuthServiceCall()
{
}

void CoAuthServiceCall::OnFinalRelease()
{
	TRACE0("CoAuthServiceCall::OnFinalRelease()\n");
	CCmdTarget::OnFinalRelease();
}

// CoAuthServiceCall message handlers
HRESULT CoAuthServiceCall::Init(BSTR bstrUrl)
{
	_ASSERT(NULL == m_spRequest); // initalize only once
	m_spRequest.CreateInstance(__uuidof(MSXML2::XMLHTTP40));

	// add sink to xml http request
	HRESULT hr = m_spRequest->put_onreadystatechange(GetIDispatch(TRUE));
	_ASSERT(SUCCEEDED(hr));

	m_eState = InitialRequest;
	m_bstrMethod = "GET"; // bstrMethod;
	m_bstrUrl = bstrUrl;
	TRACE2("  %ls: %ls (first trial)\n", (BSTR)m_bstrMethod, (BSTR)m_bstrUrl);

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
// METHOD_PROLOGUE(CoAuthServiceCall, IRenewCallback);
