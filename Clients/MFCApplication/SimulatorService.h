#pragma once

#include "..\CoAuthServiceCall.h"

class CSimulatorWf
{
public:
	static const CString FileMonikerDN4TokenResponse(LPCTSTR szClientId);
	static HRESULT GetTokenServerByDisplayName(LPCTSTR szDisplayName, oAuthLib::IAuthorize** ppAuthorize);
};

/*
* alles was irgendwie einen "Simulator" im namen hat geht gegen einen "fake" service
* the only thing you need is to derive your class from CCallbackoAuthImpl < T >
*/
class CSimulatorPing : public CoAuthServiceCall
{
public:
	HRESULT Init(LPCTSTR szClientId);

	// overrides
protected:
	/*virtual*/ HRESULT GetTokenServer(oAuthLib::IAuthorize** ppAuthorize);
#ifdef AUTHORIZATION_SERVER_SUPPORT_JSON
	/*virtual*/ void onSucceeded(web::json::value& result);
#else
	/*virtual*/ void onSucceeded();
#endif

	DECLARE_DYNCREATE(CSimulatorPing)
};
