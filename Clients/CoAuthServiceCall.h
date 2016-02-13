#pragma once

/*
* die idee an sich ist aus meiner sicht nicht so schlecht denn so koennte man auch den <company>/<product> speziellen pfad abhandeln.
* machen wir z.b. mit dem Meta auch so
*   ::SetEnvironmentVariable(_T("cticlientlocalappdata"), theServerSettings.GetCtiClientApplicationLocalDirectory().c_str());
*   ::SetEnvironmentVariable(_T("cticlientappdata"), theServerSettings.GetCtiClientApplicationRoamingDirectory().c_str());
*
* - die ansicht: voellig auf absolute FileNamen zu verzichten und alles relativ zu addressieren,
*   koennen wir getrost "ad acta" legen denn: es KANN NICHT in ein %ProgramFiles(x86)% verzeichnis geschrieben werden
*
* - andererseits gibt es inzwischen vermehrt produkte (spotify, chrome) die sich explizit lokal/per user installieren
* mal ein "per user" .msi bauen und einen pfad unter %appdata% installieren
*/
#define TOKEN_RESPONSE_USER_ENV_FMT	_T("%%appdata%%\\client_secret_%s.TokenResponse-user")
#define CLIENT_SECRET_ENV_FMT		_T("%%appdata%%\\client_secret_%s.json")

// CoAuthServiceCall command target
// diese klasse ist die MFC Version der ATL klasse (CCallbackoAuthImpl < T >)
class CoAuthServiceCall : public CCmdTarget
{
public:
	CoAuthServiceCall();
	virtual ~CoAuthServiceCall();
	virtual void OnFinalRelease();

	HRESULT Init(BSTR bstrUrl);

protected:
	DECLARE_DYNCREATE(CoAuthServiceCall)
	DECLARE_MESSAGE_MAP()

	DECLARE_DISPATCH_MAP()
	void ReadyStateChange();

protected:
	MSXML2::IXMLHTTPRequestPtr m_spRequest;

	virtual HRESULT GetTokenServer(oAuthLib::IAuthorize**) { return E_NOTIMPL; }
	virtual void onSucceeded(web::json::value& result) { }
	virtual void onFailed() {}

private:
	void Continue() {}
	void Terminate() {}

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
