// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
// #define POWERBROADCAST_TEST  2
#define WM_REQUEST			(WM_USER + 0x10)

class CSimulatorPing;

class CMainDlg :
	public CDialogImpl < CMainDlg > // https://msdn.microsoft.com/en-us/library/aa260759(v=vs.60).aspx#atlwindow_topic12
{
public:
	CMainDlg(LPCTSTR szConfigFile);

	enum { IDD = IDD_MAINDLG };

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_WTSSESSION_CHANGE, OnWTSSessionChange)
		MESSAGE_HANDLER(WM_POWERBROADCAST, OnPowerBroadcast)
#ifdef FEATURE_TASKSCHD
#else
		MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
#endif
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_RUNPING, OnBnClickedRunping)

// Custom Messages
	#ifdef MONITOR_PENDING_REQUESTS
		MESSAGE_HANDLER(WM_REQUEST, OnRequest)
	#endif
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnWTSSessionChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnPowerBroadcast(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
#ifdef MONITOR_PENDING_REQUESTS
	LRESULT OnRequest(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
#endif
#ifdef FEATURE_TASKSCHD
#else
	LRESULT OnHScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
#endif
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedRunping(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

// attributes
public:
#ifdef FEATURE_TASKSCHD
	int m_iExitCode;
#endif

private:
	MSXML2::IXMLHTTPRequest2Ptr m_spRequest2;
	CString m_strConfigFile;
	int m_TimerStressPos; // current value;
	BOOL m_bRegisterSessionNotification;
	WPARAM m_PowerBroadcast;
#ifdef POWERBROADCAST_TEST
	UINT_PTR m_timerPowerBroadcast;
#endif
#ifdef MONITOR_PENDING_REQUESTS
	typedef std::list < CSimulatorPing* > LSTPENDINGREQUESTS; // CAdapt < IUnknownPtr >
	LSTPENDINGREQUESTS m_lstPendingRequests;
#endif
	void SetTimerStressPos(int iNewPos);
	UINT_PTR m_timerStress;

	void SuspendApplication();
	void ResumeApplication();
};
