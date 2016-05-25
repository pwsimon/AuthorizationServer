#include "stdafx.h"
#include "resource.h"
#include "MainDlg.h"
#include "..\IWorkflowWebImpl.h"
#include "SimulatorService.h"

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
