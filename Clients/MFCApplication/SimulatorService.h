#pragma once

#include "..\CallbackoAuth.h"
#include "..\IWorkflowWebImpl.h"

class ATL_NO_VTABLE CSimulatorWf :
	  public CComObjectRootEx < CComSingleThreadModel >
	, public CComCoClass < CSimulatorWf >
	, public IProvideClassInfo2Impl < &CLSID_NULL, &CLSID_NULL >
	, public IWorkflowWebImpl < CSimulatorWf >
{
public:
	BEGIN_COM_MAP(CSimulatorWf)
		COM_INTERFACE_ENTRY(IProvideClassInfo)
		COM_INTERFACE_ENTRY(IProvideClassInfo2)
	END_COM_MAP()

	DECLARE_PROTECT_FINAL_CONSTRUCT()

#ifdef _DEBUG
	HRESULT FinalConstruct() { return NOERROR; }
	void FinalRelease() {}
#endif
	HRESULT Initialize(LPUNKNOWN pBrowser);
	HRESULT GetAutenticationURI(BSTR* pbstrAutenticationURI) { return E_NOTIMPL; }
	HRESULT OnAdvise(LPUNKNOWN pWebBrowser2) { return E_NOTIMPL; }

private:
	HRESULT ExchangeAndPersist();
};

/*
* alles was irgendwie einen "Simulator" im namen hat geht gegen einen "fake" service
* the only thing you need is to derive your class from CCallbackoAuthImpl < T >
*/
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

	HRESULT Init()
	{
		// base class, dynamic result
		// die ping.asp liefert je nach "Bearer: <token>" ein 200 oder im fall von expired ein 401
		return CCallbackoAuthImpl < CSimulatorPing >::Init(CComBSTR(L"http://ws-psi-win8.estos.de/procallsdk/solution/teamviewer/tvsimulator/ping.asp"));
	}

	HRESULT GetTokenServer(oAuthLib::IAuthorize** ppAuthorize) {
		const CString strMonikerName = IWorkflowWebImpl < CSimulatorWf >::FileMonikerDN4TokenResponse(_T("SimulatorClientId"));
		return IWorkflowWebImpl < CSimulatorWf >::GetTokenServerByDisplayName(strMonikerName, ppAuthorize);
	}

	void onSucceeded(const web::json::value& jsonRepCon) {
		// https://casablanca.codeplex.com/wikipage?title=JSON&referringTitle=Documentation
		ATLTRACE2(atlTraceGeneral, 0, _T("  CSimulatorPing::onSucceeded()\n"));
	}
	void onFailed() {
		ATLTRACE2(atlTraceGeneral, 0, _T("  CSimulatorPing::onFailed()\n"));
	}
};
