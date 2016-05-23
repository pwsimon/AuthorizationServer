#include "..\CallbackoAuth.h"

typedef IWorkflowWebImpl < void > CSimulatorWf;

class ATL_NO_VTABLE CSimulatorPing :
	public CComObjectRootEx < CComSingleThreadModel >
	, public CComCoClass < CSimulatorPing >
	, public IPersistImpl < CSimulatorPing >
	, public CCallbackoAuthImpl < CSimulatorPing >
{
public:
	BEGIN_COM_MAP(CSimulatorPing)
		COM_INTERFACE_ENTRY(IPersist)
		COM_INTERFACE_ENTRY_CHAIN(CCallbackoAuthImpl < CSimulatorPing >)
	END_COM_MAP()

#ifdef _DEBUG
	HRESULT FinalConstruct() { return NOERROR; }
	void FinalRelease() {}
#endif

	HRESULT Init(HWND hwndResult)
	{
		m_hwndResult = hwndResult;

		// base class, dynamic result
		// der HTTP Request liefert je nach "Bearer: <token>" ein 200 oder im fall von expired ein 401
		return CCallbackoAuthImpl < CSimulatorPing >::Init(_T("GET"), _T("http://simulatorauthserver-1310.appspot.com/ping"));
	}

	HRESULT GetTokenServer(oAuthLib::IAuthorize** ppAuthorize) {
		const CString strMonikerName = CSimulatorWf::FileMonikerDN4TokenResponse(_T("SimulatorClientId"));
		return CSimulatorWf::GetTokenServerByDisplayName(strMonikerName, ppAuthorize);
	}

#ifdef AUTHORIZATION_SERVER_SUPPORT_JSON
	void onSucceeded(const Json::Value& jsonRepCon) {
		ATLTRACE2(atlTraceGeneral, 0, _T("CSimulatorPing::onSucceeded()\n"));
	}
#else
	void onSucceeded() {
		::SendMessage(m_hwndResult, WM_SETTEXT, 0, (LPARAM) _T("onSucceeded"));
		// die pruefung auf HTTP-Status == 200 (OK )ist die bedingung das hier ueberhaupt onSucceeded() aufgerufen wird
		ATLTRACE2(atlTraceGeneral, 0, _T("CSimulatorPing::onSucceeded() HTTP Status: 0x%d, %ls\n"), m_spRequest->status, (BSTR)m_spRequest->statusText);

		MSXML2::IXMLDOMDocument2Ptr spXML(m_spRequest->responseXML);
		ATLTRACE2(atlTraceGeneral, 1, _T("CSimulatorPing::onSucceeded() Result: %ls\n"), (BSTR)spXML->documentElement->xml);
	}
#endif

	void onFailed() {
		::SendMessage(m_hwndResult, WM_SETTEXT, 0, (LPARAM)_T("OnFailed"));
		// die ausgabe von HTTP-Status, statusText, ... ist im fehlerfall wichtiger/sinvoller als im gut fall
		ATLTRACE2(atlTraceGeneral, 0, _T("CSimulatorPing::onFailed() HTTP Status: 0x%d, %ls\n"), m_spRequest->status, (BSTR)m_spRequest->statusText);
	}

private:
	HWND m_hwndResult;
};
