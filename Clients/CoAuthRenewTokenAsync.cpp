// CoAuthRenewTokenAsync.cpp : implementation file
//

#include "stdafx.h"
#include "CoAuthRenewTokenAsync.h"
#include "CoAuthServiceCall.h"

// CoAuthRenewTokenAsync
IMPLEMENT_DYNCREATE(CoAuthRenewTokenAsync, CCmdTarget)

BEGIN_MESSAGE_MAP(CoAuthRenewTokenAsync, CCmdTarget)
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CoAuthRenewTokenAsync, CCmdTarget)
	DISP_FUNCTION_ID(CoAuthRenewTokenAsync, "OnReadyStateChange", 0, ReadyStateChange, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()

// IDispatch implementation
void CoAuthRenewTokenAsync::ReadyStateChange()
{
	InternalAddRef(); // lock protect your instance
	try
	{
		TRACE1("CoAuthRenewTokenAsync::IXMLDOMDocumentEvents::OnReadyStateChange() readyState: 0x%.8x\n", m_spRequest->readyState);
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

	InternalRelease(); // unlock instance, may the last one
}

CoAuthRenewTokenAsync::CoAuthRenewTokenAsync()
{
	__super::EnableAutomation();
}

void CoAuthRenewTokenAsync::OnFinalRelease()
{
	TRACE0("CoAuthRenewTokenAsync::OnFinalRelease()\n");
	CCmdTarget::OnFinalRelease();
}

// CoAuthRenewTokenAsync message handlers
HRESULT CoAuthRenewTokenAsync::Init(oAuthLib::IAuthorize* pAuthorize, oAuthLib::IRenewCallback* pRenewCallback)
{
	ASSERT(NULL == m_spRequest); // initalize only once
	ASSERT(NULL == m_spRenewCallback);
	ASSERT(NULL != pRenewCallback);
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
			m_spRequest->put_onreadystatechange(GetIDispatch(FALSE));

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
