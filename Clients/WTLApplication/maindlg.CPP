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

#define SLIDER_MIN    0
#define SLIDER_MAX    5
#define TIMER_STRESS  1

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

CMainDlg::CMainDlg(LPCTSTR szConfigFile) :
	m_strConfigFile(szConfigFile)
{
}
#endif

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow();

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
	if(m_strConfigFile.GetLength())
		PostMessage(WM_COMMAND, MAKEWPARAM(IDC_RUNPING, 0), 0);
#else
	m_TimerStressPos = 0;
#endif

	return TRUE;
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
	switch (wParam)
	{
		case TIMER_STRESS:
			OnBnClickedRunping(0, 0, NULL, bHandled);
			break;
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
			SetTimer(TIMER_STRESS, 5000 * (6 - m_TimerStressPos), NULL);
		else
			KillTimer(TIMER_STRESS);
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
	CComObject < CSimulatorPing >* pTask = NULL;
	HRESULT hr = CComObject < CSimulatorPing >::CreateInstance(&pTask);
#ifdef FEATURE_TASKSCHD
	pTask->Load((LPCTSTR) m_strConfigFile, STGM_READ | STGM_SHARE_DENY_WRITE);
	pTask->Init(m_hWnd);
#else
	pTask->Init(GetDlgItem(IDC_STATUS));
#endif

	return 0;
}
