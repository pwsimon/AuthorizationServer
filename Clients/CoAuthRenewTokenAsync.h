#pragma once

class CoAuthServiceCall;

// CoAuthRenewTokenAsync command target
class CoAuthRenewTokenAsync : public CCmdTarget
{
public:
	CoAuthRenewTokenAsync();
	virtual void OnFinalRelease();
	HRESULT Init(oAuthLib::IAuthorize* pAuthorize, oAuthLib::IRenewCallback* pRenewCallback);

protected:
	DECLARE_DYNCREATE(CoAuthRenewTokenAsync)
	DECLARE_MESSAGE_MAP()

	DECLARE_DISPATCH_MAP()
	void ReadyStateChange();

private:
	oAuthLib::IAuthorizePtr m_spAuthorize;
	oAuthLib::IRenewCallbackPtr m_spRenewCallback;
	MSXML2::IXMLHTTPRequestPtr m_spRequest;
};
