#include "stdafx.h"
#include "resource.h"
#include "MainDlg.h"
#include "..\IWorkflowWebImpl.h"
#include "SimulatorService.h"

#ifdef FEATURE_TASKSCHD
// Include the task header file.
#include <taskschd.h>
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")
#pragma comment(lib, "credui.lib")
#endif

#define SLIDER_MIN           0
#define SLIDER_MAX           5
#define TIMER_STRESS         1

#ifdef FEATURE_TASKSCHD
// access the Scheduler service itself
HRESULT AllocScheduler(LPCWSTR szFolder, ITaskFolder** ppRootFolder)
{
	CComPtr <ITaskService> spService;
	HRESULT hr = spService.CoCreateInstance(CLSID_TaskScheduler);
	hr = spService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
	CComPtr <ITaskFolder> spRootFolder;
	hr = spService->GetFolder(_bstr_t(szFolder), ppRootFolder);
	return hr;
}

HRESULT EnableExistingTask(LPCWSTR szTaskName, VARIANT_BOOL vbEnable, ITaskFolder* pRootFolder)
{
	// query specific task
	_bstr_t bstrTaskName(szTaskName);
	CComPtr <ITaskDefinition> spTaskDefinition;
	HRESULT hr = E_FAIL;

	{
		/*
		* wirklich wichtig ist die ITaskDefinition und nicht die IRegisteredTask
		* die ITaskDefinition brauchen wir spaeter um unsere aenderungen zu speichern
		*/
		CComPtr <IRegisteredTask> spRTExisting;
		hr = pRootFolder->GetTask(bstrTaskName, &spRTExisting);
		hr = spRTExisting->put_Enabled(vbEnable);
	}
	return hr;
}

HRESULT ModifyExistingTask(LPCWSTR szTaskName, ITaskFolder* pRootFolder)
{
	// query specific task
	_bstr_t bstrTaskName(szTaskName);
	CComPtr <ITaskDefinition> spTaskDefinition;
	HRESULT hr = E_FAIL;

	{
		/*
		* wirklich wichtig ist die ITaskDefinition und nicht die IRegisteredTask
		* die ITaskDefinition brauchen wir spaeter um unsere aenderungen zu speichern
		*/
		CComPtr <IRegisteredTask> spRTExisting;
		hr = pRootFolder->GetTask(bstrTaskName, &spRTExisting);
		spRTExisting->get_Definition(&spTaskDefinition);

#ifdef _DEBUG
		COleDateTime dtLastRunTime;
		spRTExisting->get_LastRunTime(&dtLastRunTime.m_dt);
		CString strLastRunTime = dtLastRunTime.Format();
#endif
	}

	{
		CComPtr <IActionCollection> spActionCollection;
		spTaskDefinition->get_Actions(&spActionCollection);

#ifdef _DEBUG
		long lCount = 0; // state before
		hr = spActionCollection->get_Count(&lCount);

		{
			// query for simple action
			CComPtr <IAction> spAction;
			spActionCollection->get_Item(1, &spAction);

			CComQIPtr <IExecAction> spExecAction = spAction;
		}
#endif

		{
			// create additional simple action
			CComPtr <IAction> spAction;
			spActionCollection->Create(TASK_ACTION_EXEC, &spAction);

#ifdef _DEBUG
			lCount = 0; // count after insert/create
			hr = spActionCollection->get_Count(&lCount);
#endif

			CComQIPtr <IExecAction> spExecAction = spAction;
			spExecAction->put_Path(_bstr_t(L"C:\\Users\\psi\\Source\\Repos\\AuthorizationServer\\Debug\\WTLApplication.exe"));
		}

		/*
		* save your changes (persistence) by update TaskDefinition
		* CAUTION:
		*   needs "Administrative" rights (elevation)
		*/
		CComPtr <IRegisteredTask> spRTUpdated;
		hr = pRootFolder->RegisterTaskDefinition(bstrTaskName, spTaskDefinition, TASK_CREATE_OR_UPDATE, _variant_t(), _variant_t(), TASK_LOGON_INTERACTIVE_TOKEN, _variant_t(), &spRTUpdated);
	}

	return hr;
}

HRESULT NewTask(ITaskFolder* pRootFolder, LPCWSTR szTaskName)
{
	CComPtr <ITaskService> spService;
	HRESULT hr = spService.CoCreateInstance(CLSID_TaskScheduler);
	hr = spService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());

	CComPtr <ITaskDefinition> spTaskDefinition;
	hr = spService->NewTask(0, &spTaskDefinition);

	return hr;
}
#endif

CMainDlg::CMainDlg(LPCTSTR szConfigFile) :
	m_strConfigFile(szConfigFile),
#ifdef POWERBROADCAST_TEST
	m_timerPowerBroadcast(0),
#endif
	m_timerStress(0),
	m_PowerBroadcast(-1)
{
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow();

	if (!m_strConfigFile.IsEmpty())
	{
		TCHAR szFileName[_MAX_PATH];
		_tsplitpath(m_strConfigFile, NULL, NULL, szFileName, NULL);
		SetWindowText(szFileName);
	}

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	CTrackBarCtrl ctlTrack(GetDlgItem(IDC_SLIDER));
	ctlTrack.SetRange(SLIDER_MIN, SLIDER_MAX);

#ifdef FEATURE_TASKSCHD
	/* CComPtr <ITaskFolder> spRootFolder;
	HRESULT hr = AllocScheduler(L"\\AuthorisationClient", &spRootFolder);
	hr = EnableExistingTask(L"WebService Ping", VARIANT_FALSE, spRootFolder);
	// hr = ModifyExistingTask(L"WebService Ping", spRootFolder);
	*/

	/* if(m_strConfigFile.GetLength())
		PostMessage(WM_COMMAND, MAKEWPARAM(IDC_RUNPING, 0), 0);
	*/

	/*
	* fuer einen kurzen test wakeup mit TaskScheduler beenden wir uns einfach und fuehren KEINEN ping durch
	PostMessage(WM_CLOSE, 0, 0);
	*/

#else
	m_bRegisterSessionNotification = ::WTSRegisterSessionNotification(m_hWnd, NOTIFY_FOR_THIS_SESSION);
	m_TimerStressPos = 0;
#endif

#ifdef POWERBROADCAST_TEST
	// siehe auch:
	//   ResumeApplication()
	m_timerPowerBroadcast = SetTimer(POWERBROADCAST_TEST, 1000 * 60, NULL);
#endif

	return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
#ifdef FEATURE_TASKSCHD
#else
	if (m_bRegisterSessionNotification)
		::WTSUnRegisterSessionNotification(m_hWnd);
#endif

#ifdef POWERBROADCAST_TEST
	if (m_timerPowerBroadcast)
	{
		KillTimer(m_timerPowerBroadcast);
		m_timerPowerBroadcast = 0;
	}
#endif

	if (m_timerStress)
	{
		KillTimer(m_timerStress);
		m_timerStress = 0;
	}

#ifdef MONITOR_PENDING_REQUESTS
	// _ASSERT(0 == m_lstPendingRequests.size()); // MemoryLeak
	// mit einem einfachen/automatischen sind schonmal alle MemoryLeak's gefixed
	// step2: (optional) die netzwerk resourcen freigeben
	// bzw. NUR die netzwerk resourcen freigeben und mit automatischen release belohnt werden???
	for each (LSTPENDINGREQUESTS::value_type pRequest in m_lstPendingRequests)
	{
		pRequest->Abort();
	}
#endif

	return 0L;
}

LRESULT CMainDlg::OnWTSSessionChange(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	ATLTRACE2(atlTraceGeneral, 1, _T("Enter CMainDlg::OnWTSSessionChange() 0x%.8x, 0x%.8x\n"), wParam, lParam);
	switch (wParam)
	{
	case WTS_SESSION_LOCK: // 7
		ATLTRACE2(atlTraceGeneral, 0, _T("  CMainDlg::OnWTSSessionChange() WTS_SESSION_LOCK, %s\n"), (LPCTSTR)COleDateTime::GetCurrentTime().Format());
		break;
	case WTS_SESSION_UNLOCK: // 8
		ATLTRACE2(atlTraceGeneral, 0, _T("  CMainDlg::OnWTSSessionChange() WTS_SESSION_UNLOCK, %s\n"), (LPCTSTR)COleDateTime::GetCurrentTime().Format());
		break;
	default:
		ATLTRACE2(atlTraceGeneral, 0, _T("  CMainDlg::OnWTSSessionChange() 0x%.8x, 0x%.8x\n"), wParam, lParam);
		break;
	}
	return 0;
}

/*
* unter "Event Viewer (local)\Windows Logs\System"
*   source: Kernel-Power                    // Sleep Reason: System Idle
*   source: Power-Troubleshooter,           // Wake Source: Device -Intel(R) Ethernet Connection (2) I219-V
*
* siehe auch:
*   RegisterPowerSettingNotification function, https://msdn.microsoft.com/de-de/library/windows/desktop/aa373196(v=vs.85).aspx
*
* mit dem ACTIVE Management Suspend/Resume-Application ist garantiert dass:
* das ERSTE Timer event (gem. einstellung 25 Sek.) NACH dem OnPowerBroadcast(PBT_APMRESUMEAUTOMATIC) verarbeitet wird.
* dennoch versagt dieser ERSTE request regelmaessig UND ich bekomme noch nicht mal ein feed back.
* kann man die netzwerkschnittstelle explizit auf ready pruefen bzw restarten?
*/
LRESULT CMainDlg::OnPowerBroadcast(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
#ifdef _DEBUG
	/*
	* WTSDisconnected (4) => WinStation logged on without client -> KEINE antwort auf XMLHTTPRequest???
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
		ATLTRACE2(atlTraceGeneral, 0, _T("Enter CMainDlg::OnPowerBroadcast() wParam: 0x%.8x, WTS_CONNECTSTATE_CLASS: 0x%.8x\n"), wParam, *pResult);
		::WTSFreeMemory(pResult);
	}
	else
		ATLTRACE2(atlTraceGeneral, 0, _T("Enter CMainDlg::OnPowerBroadcast() wParam: 0x%.8x, WTS_CONNECTSTATE_CLASS: Failed\n"), wParam);
#endif

	m_PowerBroadcast = wParam;
	switch (wParam)
	{
		/*
		* mit suspend durch: a.) den PowerPlan oder b.) durch den user
		* wird die Anwendung offensichtlich mit einem PBT_APMSUSPEND benachrichtigt
		* der folgende request durch "Msxml2.ServerXMLHTTP.6.0"
		*   findet aber kein ende (receive status)???, geht noch nicht mal raus???
		* der folgende request durch "Msxml2.XMLHTTP.6.0"
		*   endet mit HTTP Status: 0x12007
		*
		* ich vermute das liegt an der fehlenden desktop-session
		*/
	case PBT_APMSUSPEND:         // 0x04, System is suspending operation.
	{
		ATLTRACE2(atlTraceGeneral, 0, _T("CMainDlg::OnPowerBroadcast() PBT_APMSUSPEND, %s\n"), (LPCTSTR)COleDateTime::GetCurrentTime().Format());
		SuspendApplication();

		/* if (m_strConfigFile.GetLength())
		PostMessage(WM_COMMAND, MAKEWPARAM(IDC_RUNPING, 0), 0); */
	}
	break;

	/*
	* mit dem wakeup durch die NetworkCard wird die Anwendung offensichtlich mit einem PBT_APMRESUMEAUTOMATIC benachrichtigt
	* der folgende request durch XMLHTTPRequest findet aber kein ende (receive status)???
	*/
	case PBT_APMRESUMEAUTOMATIC: // 0x12, This message is sent every time the system resumes.
	{
		ATLTRACE2(atlTraceGeneral, 0, _T("CMainDlg::OnPowerBroadcast() PBT_APMRESUMEAUTOMATIC, %s\n"), (LPCTSTR)COleDateTime::GetCurrentTime().Format());
		ResumeApplication();

		/* if (m_strConfigFile.GetLength())
		PostMessage(WM_COMMAND, MAKEWPARAM(IDC_RUNPING, 0), 0); */
	}
	break;

	case PBT_APMSTANDBY: // 0x05
	case PBT_APMPOWERSTATUSCHANGE:
	default:
		ATLTRACE2(atlTraceGeneral, 0, _T("CMainDlg::OnPowerBroadcast() wParam: 0x%.8x, %s\n"), wParam, (LPCTSTR)COleDateTime::GetCurrentTime().Format());
		break;
	}

	return 0;
}

#ifdef MONITOR_PENDING_REQUESTS
/*
* wird ueber die gesamte lebensdauer eines CSimulatorPing NUR einmal aufgerufen
* mit JEDEM NEUEN CSimulatorPing wird m_iExitCode wieder mit 2 initialisiert
*/
LRESULT CMainDlg::OnRequest(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	/*
	* dieses hin und her von Methode zu Code ist nicht schoen
	* CoAuthServiceCall::onSucceeded() -> CMainDlg::WM_USER + 0x10, wParam -> ::ExitCode(<number>)
	* den liefern wir im batch mode auch als exit code
	*/
	m_iExitCode = wParam;
	switch (wParam)
	{
		case 0: // auf ebene ExitCode ist 0 typisch fuer succeeded
		{
			SetDlgItemText(IDC_STATUS, (LPCTSTR)_T("Succeeded"));

			// manage EXPLIZIT remove
			// see also:
			//   CMainDlg::OnDestroy() implizit remove (Timeout)
			LSTPENDINGREQUESTS::const_iterator iterFound = std::find(m_lstPendingRequests.begin(), m_lstPendingRequests.end(), (LSTPENDINGREQUESTS::value_type) lParam);
			m_lstPendingRequests.erase(iterFound);
			break;
		}
		case 1: // und 1 fuer failed
		{
			SetDlgItemText(IDC_STATUS, (LPCTSTR)_T("Failed"));

			// manage EXPLIZIT remove
			// see also:
			//   CMainDlg::OnDestroy() implizit remove (Timeout)
			LSTPENDINGREQUESTS::const_iterator iterFound = std::find(m_lstPendingRequests.begin(), m_lstPendingRequests.end(), (LSTPENDINGREQUESTS::value_type) lParam);
			m_lstPendingRequests.erase(iterFound);
			break;
		}
		case 2: // Loading
			SetDlgItemText(IDC_STATUS, (LPCTSTR)_T("Loading"));
			break;
		default:
			ATLTRACE2(atlTraceGeneral, 0, _T("ERROR: Unknown Command\n"));
			break;
	}

	return 0;
}
#endif

void CMainDlg::SuspendApplication()
{
#ifdef POWERBROADCAST_TEST
	if (m_timerPowerBroadcast)
	{
		KillTimer(m_timerPowerBroadcast);
		m_timerPowerBroadcast = 0;
	}
#endif

	if (m_timerStress)
	{
		KillTimer(m_timerStress);
		m_timerStress = 0;
	}
}

void CMainDlg::ResumeApplication()
{
#ifdef POWERBROADCAST_TEST
	_ASSERT(0 == m_timerPowerBroadcast);
	m_timerPowerBroadcast = SetTimer(POWERBROADCAST_TEST, 1000 * 60, NULL);
#endif

	// der fachliche zustand dieses timers (abgesehen von den OS resourcen) wird in m_TimerStressPos persistiert
	// solange der speicher mit suspend NICHT verloren geht koennen wir den timer (OS resourcen) daraus wiederherstellen
	_ASSERT(0 == m_timerStress);
	if(m_TimerStressPos)
		m_timerStress = SetTimer(TIMER_STRESS, 5000 * (6 - m_TimerStressPos), NULL);
}

#ifdef FEATURE_TASKSCHD
#else
// WM_HSCROLL message, https://msdn.microsoft.com/en-us/library/windows/desktop/bb787575(v=vs.85).aspx
LRESULT CMainDlg::OnHScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	// Get the minimum and maximum scroll-bar positions.
	CTrackBarCtrl ctlTrack(GetDlgItem(IDC_SLIDER));
	int curpos = ctlTrack.GetPos();

	switch (LOWORD(wParam))
	{
	case SB_LEFT:      // Scroll to far left.
		_ASSERT(SLIDER_MIN == curpos);
		SetTimerStressPos(SLIDER_MIN); // Pos1 key
		break;

	case SB_RIGHT:      // Scroll to far right.
		_ASSERT(SLIDER_MAX == curpos);
		SetTimerStressPos(SLIDER_MAX); // End key
		break;

	case SB_ENDSCROLL:   // End scroll.
		break;

	case SB_LINELEFT:      // Scroll left.
		SetTimerStressPos(curpos);
		break;

	case SB_LINERIGHT:   // Scroll right.
		SetTimerStressPos(curpos);
		break;

	case SB_PAGELEFT:    // Scroll one page left.
		SetTimerStressPos(curpos); // click left side of slider
		break;

	case SB_PAGERIGHT:   // Scroll one page right.
		SetTimerStressPos(curpos); // click right side of slider
		break;

	case SB_THUMBPOSITION: // Scroll to absolute position. nPos is the position
		SetTimerStressPos(HIWORD(wParam));
		break;

	case SB_THUMBTRACK: // Drag scroll box to specified position. nPos is the
		// SetTimerStressPos(HIWORD(wParam)); // of the scroll box at the end of the drag operation.
		break;
	}

	return 0;
}

// WM_TIMER message, https://msdn.microsoft.com/en-us/library/windows/desktop/ms644902(v=vs.85).aspx
LRESULT CMainDlg::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
/*
* ALLE laufenden Timer der Anwendung sollten mit dem OnPowerBroadcast(PBT_APMSUSPEND) gekilled werden.
* sonst kann es passieren das die WM_TIMER routine(n) beim "Resume" VOR dem OnPowerBroadcast(PBT_APMRESUMEAUTOMATIC) handler laufen!
* a.) entweder Timer canceln (siehe SuspendApplication/ResumeApplication()) ODER
* b.) die Timer routine sollten/muessen diesen Unterschied erkennen/auswerten
*
* warum ist das NICHT gut wenn Timer VOR dem OnPowerBroadcast(PBT_APMRESUMEAUTOMATIC) laufen?
* typischerweise versagen z.B. XMLHTTPRequests
*/
	ATLTRACE2(atlTraceGeneral, 1, _T("CMainDlg::OnTimer() m_PowerBroadcast: 0x%.8x, %s, 0x%.8x\n"), m_PowerBroadcast, (LPCTSTR)COleDateTime::GetCurrentTime().Format(), wParam);
	_ASSERT(PBT_APMSUSPEND != m_PowerBroadcast); // wir haben uns fuer ein Suspend/Resume-Application() entschieden

	switch (wParam)
	{
		case TIMER_STRESS:
			ATLTRACE2(atlTraceGeneral, 0, _T("  CMainDlg::OnTimer() m_PowerBroadcast: 0x%.8x, %s, TIMER_STRESS\n"), m_PowerBroadcast, (LPCTSTR)COleDateTime::GetCurrentTime().Format());
			OnBnClickedRunping(0, 0, NULL, bHandled);
			break;
#ifdef POWERBROADCAST_TEST
		case POWERBROADCAST_TEST:
			ATLTRACE2(atlTraceGeneral, 0, _T("  CMainDlg::OnTimer() m_PowerBroadcast: 0x%.8x, %s, POWERBROADCAST_TEST\n"), m_PowerBroadcast, (LPCTSTR)COleDateTime::GetCurrentTime().Format());
			break;
#endif
	}
	return 0;
}

/*
* wir verwenden kueftig die Task Scheduler API
* https://msdn.microsoft.com/en-us/library/windows/desktop/aa383614(v=vs.85).aspx
* bzw. SetThreadExecutionState function https://msdn.microsoft.com/de-de/library/windows/desktop/aa373208(v=vs.85).aspx
* und gehen damit dem SleepModus/PowerSaving options aus dem weg
*/
void CMainDlg::SetTimerStressPos(int iNewPos)
{
	if (m_TimerStressPos != iNewPos) // aenderungserkennung
	{
		m_TimerStressPos = iNewPos;
		if (0 < m_TimerStressPos)
			m_timerStress = SetTimer(TIMER_STRESS, 5000 * (6 - m_TimerStressPos), NULL);
		else
		{
			KillTimer(m_timerStress);
			m_timerStress = 0;
		}
	}
}
#endif

LRESULT CMainDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CSimpleDialog<IDD_ABOUTBOX, FALSE> dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnBnClickedRunping(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
#ifdef XMLHTTP_WINHTTP
	class ATL_NO_VTABLE CMyCallback :
		  public CComObjectRootEx < CComSingleThreadModel >
		, public WinHttp::IWinHttpRequestEvents
	{
		BEGIN_COM_MAP(CMyCallback)
			COM_INTERFACE_ENTRY(WinHttp::IWinHttpRequestEvents)
		END_COM_MAP()

		STDMETHOD_(void, OnResponseStart)(/*[in]*/ long Status, /*[in]*/ BSTR ContentType) { }
		STDMETHOD_(void, OnResponseDataAvailable)(/*[in]*/ SAFEARRAY** Data) { }
		STDMETHOD_(void, OnResponseFinished)() { }
		STDMETHOD_(void, OnError)(/*[in]*/ long ErrorNumber, /*[in]*/ BSTR ErrorDescription) { }
	};
	CComObject < CMyCallback >* pCB = NULL;
	CComObject < CMyCallback >::CreateInstance(&pCB);

	try
	{
		WinHttp::IWinHttpRequestPtr spRequest(__uuidof(WinHttp::WinHttpRequest));
		IConnectionPointContainerPtr spCC(spRequest);
		IConnectionPointPtr spCP;
		HRESULT hr = spCC->FindConnectionPoint(__uuidof(WinHttp::IWinHttpRequestEvents), &spCP);
		DWORD dwCookie = 0L;
		hr = spCP->Advise(pCB->GetUnknown(), &dwCookie);

		hr = spRequest->Open(L"GET", L"http://www.estos.de/", VARIANT_FALSE);

		hr = spRequest->Send();
		_bstr_t bstrResult = spRequest->ResponseText;
		ATLTRACE2(atlTraceCOM, 0, _T("finish\n"));
	}
	catch (const _com_error& e)
	{
	}
#endif

#ifdef XMLHTTP_MSHTML
	try
	{
		IUnknownPtr spXHR(__uuidof(MSHTML::XDomainRequest));

		MSHTML::IHTMLXDomainRequestFactoryPtr spFactory;
		HRESULT hr = spFactory.CreateInstance(__uuidof(MSHTML::XDomainRequestFactory));
		MSHTML::IHTMLXMLHttpRequestPtr spRequest(spFactory->create());
		// spRequest->onreadystatechange = NULL;
		spRequest->open(L"GET", L"http://www.estos.de/", VARIANT_FALSE);

		MSHTML::IHTMLXMLHttpRequest2Ptr spRequest2(spRequest);
		// spRequest2->ontimeout = NULL;

		spRequest->send();
		_bstr_t bstrResult = spRequest->responseText;
		ATLTRACE2(atlTraceCOM, 0, _T("finish\n"));
	}
	catch (const _com_error& e)
	{
	}
#endif

#if 0
	class ATL_NO_VTABLE CMyCallback :
		  public CComObjectRootEx < CComSingleThreadModel >
		, public MSXML2::IXMLHTTPRequest2Callback
	{
		BEGIN_COM_MAP(CMyCallback)
			COM_INTERFACE_ENTRY(MSXML2::IXMLHTTPRequest2Callback)
		END_COM_MAP()

		STDMETHOD(raw_OnRedirect) (MSXML2::IXMLHTTPRequest2* pXHR, LPWSTR pwszRedirectUrl) { return E_NOTIMPL; }
		STDMETHOD(raw_OnHeadersAvailable)(MSXML2::IXMLHTTPRequest2* pXHR, unsigned long dwStatus, LPWSTR pwszStatus) { return E_NOTIMPL; }
		STDMETHOD(raw_ondataavailable)(MSXML2::IXMLHTTPRequest2* pXHR, struct ISequentialStream* pResponseStream) { return E_NOTIMPL; }
		STDMETHOD(raw_OnResponseReceived)(MSXML2::IXMLHTTPRequest2* pXHR, struct ISequentialStream* pResponseStream) { return E_NOTIMPL; }
		STDMETHOD(raw_OnError)(MSXML2::IXMLHTTPRequest2* pXHR, HRESULT hrError) { return E_NOTIMPL; }
	};
	CComObject < CMyCallback >* pCB = NULL;
	CComObject < CMyCallback >::CreateInstance(&pCB);

	try
	{
		MSXML2::IXMLHTTPRequestPtr spRequest;
		// HRESULT hr = spRequest.CreateInstance(L"Msxml2.ServerXMLHTTP.6.0");
		HRESULT hr = spRequest.CreateInstance(L"Msxml2.XMLHTTP.6.0");
		MSXML2::IXMLHTTPRequest2Ptr spRequest2(spRequest);
		MSXML2::IServerXMLHTTPRequestPtr spServerRequest(spRequest);
		MSXML2::IServerXMLHTTPRequest2Ptr spServerRequest2(spRequest);

		spRequest2->open(L"GET", L"http://www.estos.de/", pCB, NULL, NULL, NULL, NULL);
		// spRequest2->setProperty(MSXML2::XHR_PROP_TIMEOUT, 1000);
		spRequest2->send(NULL, 0);
	}
	catch (const _com_error& e)
	{
		CComBSTR bstrMsg = e.ErrorMessage();
		HRESULT hr = e.Error();
	}
#endif

	CComObject < CSimulatorPing >* pTask = NULL;
	HRESULT hr = CComObject < CSimulatorPing >::CreateInstance(&pTask);
	if(!m_strConfigFile.IsEmpty())
		hr = pTask->Load((LPCTSTR)m_strConfigFile, STGM_READ | STGM_SHARE_DENY_WRITE);

#ifdef MONITOR_PENDING_REQUESTS
	m_lstPendingRequests.push_back(pTask); // IUnknownPtr(pTask->GetUnknown())
#endif

	pTask->Init(m_hWnd);

	return 0;
}
