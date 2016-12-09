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
	m_TimerStressPos = 0;

#ifdef FEATURE_TASKSCHD
	// access the Scheduler service itself
	{
		CComPtr <ITaskService> spService;
		HRESULT hr = spService.CoCreateInstance(CLSID_TaskScheduler);
		hr = spService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
		CComPtr <ITaskFolder> spRootFolder;
		hr = spService->GetFolder(_bstr_t(L"\\"), &spRootFolder);

		{
			// query specific task
			CComPtr <IRegisteredTask> spRegisteredTask;
			hr = spRootFolder->GetTask(_bstr_t(L"Adobe Acrobat Update Task"), &spRegisteredTask);
			COleDateTime dtLastRunTime;
			spRegisteredTask->get_LastRunTime(&dtLastRunTime.m_dt);
			CString strLastRunTime = dtLastRunTime.Format();

			{
				// handle actions of this task
				CComPtr <ITaskDefinition> spTaskDefinition;
				spRegisteredTask->get_Definition(&spTaskDefinition);
				CComPtr <IActionCollection> spActionCollection;
				spTaskDefinition->get_Actions(&spActionCollection);

				long lCount = 0; // state before
				hr = spActionCollection->get_Count(&lCount);

				// simple action
				CComQIPtr <IExecAction> spExecAction;
				{
					CComPtr <IAction> spAction;
					spActionCollection->get_Item(0, &spAction);
					spExecAction = spAction;
				}

				// Add additional COM Action instead of
				CComQIPtr <IComHandlerAction> spComHandlerAction;
				{
					CComPtr <IAction> spAction;
					spActionCollection->Create(/*TASK_ACTION_EXEC*/ TASK_ACTION_COM_HANDLER, &spAction);
					spAction->put_Id(_bstr_t(L"using ECtiClient"));

					spComHandlerAction = spAction;
					// spComHandlerAction->put_ClassId(_bstr_t(L"{ACA2A1DD-F3B2-4C70-878B-FEEA2B32DDD0}")); // This value must be in the format "{00000000-0000-0000-0000-000000000000}". In .NET, this is the Guid.ToString("B") format and under Win32 use StringFromGUID2.
				}

				lCount = 0; // state after
				hr = spActionCollection->get_Count(&lCount);

				// save your chages (persistence)
				// hr = spTaskDefinition->put_Actions(spActionCollection);
			}
		}

		/*
		class CTaskHandler public ITaskHandler
		{
		// ITaskHandler interface, https://msdn.microsoft.com/de-de/library/windows/desktop/aa381370(v=vs.85).aspx
		STDMETHOD(Pause)() { ATLTRACE_NOTIMPL(); }
		STDMETHOD(Start)()
		{
		GetActiveObject()
		}
		}
		*/
	}
#endif

	return TRUE;
}

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

LRESULT CMainDlg::OnBnClickedRunping(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CComObject < CSimulatorPing >* pTask = NULL;
	HRESULT hr = CComObject < CSimulatorPing >::CreateInstance(&pTask);
	pTask->Init(GetDlgItem(IDC_STATUS));

	return 0;
}
