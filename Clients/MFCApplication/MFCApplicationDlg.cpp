// MFCApplicationDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MFCApplication.h"
#include "MFCApplicationDlg.h"
#include "afxdialogex.h"
#include "SimulatorService.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif
#define TIMER_GARBAGE 0
#define TIMER_STRESS  1

// CAboutDlg dialog used for App About
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

// CMFCApplicationDlg dialog
CMFCApplicationDlg::CMFCApplicationDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMFCApplicationDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCApplicationDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STRESSTIMER, m_sldrStressTimer);
}



BEGIN_MESSAGE_MAP(CMFCApplicationDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_RUNPING, &CMFCApplicationDlg::OnBnClickedRunping)
END_MESSAGE_MAP()

// CMFCApplicationDlg message handlers
BOOL CMFCApplicationDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE); // Set big icon
	SetIcon(m_hIcon, FALSE); // Set small icon

	SetTimer(TIMER_GARBAGE, 5000, NULL);
	m_sldrStressTimer.SetRange(0, 5);
	m_TimerStressPos = 0;
	return TRUE; // return TRUE  unless you set the focus to a control
}

void CMFCApplicationDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}

	if (0 < (nID & SC_CLOSE))
	{
		if (0 < m_lstCmd.size())
		{
			// if pending commands. SC_CLOSE will be ignored
			ATLTRACE2(atlTraceGeneral, 0, _T("SC_CLOSE will be ignored\n"));
		}
		else
			CDialogEx::OnSysCommand(nID, lParam);
	}

	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void CMFCApplicationDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMFCApplicationDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMFCApplicationDlg::OnBnClickedRunping()
{
/*
* the following TWO simple lines are the core of this sample
* create a request and execute them.

* all of the other stuff is only to perform garbage collection, repeat requests frequently, ...
*/
	CSimulatorPing* pSimulatorPing = DYNAMIC_DOWNCAST(CSimulatorPing, RUNTIME_CLASS(CSimulatorPing)->CreateObject());
	ASSERT(1 == pSimulatorPing->m_dwRef); // MFC Standard
	pSimulatorPing->Init(_T("SimulatorClientId"));

	m_lstCmd.push_back(pSimulatorPing);
}

void CMFCApplicationDlg::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
		case TIMER_GARBAGE:
		{
			if (m_lstCmd.size())
			{
				CSimulatorPing* pCmd = DYNAMIC_DOWNCAST(CSimulatorPing, m_lstCmd.front());
				if (NULL != pCmd)
				{
					if (CoAuthServiceCall::Finish == pCmd->GetState())
					{
						pCmd->Dispose();
						pCmd->ExternalRelease();
						m_lstCmd.pop_front();
					}
				}
			}
		}
		break;

		case TIMER_STRESS:
			OnBnClickedRunping();
			break;
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CMFCApplicationDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// Get the minimum and maximum scroll-bar positions.
	int minpos = 0;
	int maxpos = 5;
	int curpos = m_sldrStressTimer.GetPos();

	switch (nSBCode)
	{
	case SB_LEFT:      // Scroll to far left.
		curpos = minpos;
		break;

	case SB_RIGHT:      // Scroll to far right.
		curpos = maxpos;
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
	{
	}
	break;

	case SB_PAGERIGHT:      // Scroll one page right.
	{
	}
	break;

	case SB_THUMBPOSITION: // Scroll to absolute position. nPos is the position
		SetTimerStressPos(nPos);
		break;

	case SB_THUMBTRACK:   // Drag scroll box to specified position. nPos is the
		curpos = nPos;      // of the scroll box at the end of the drag operation.
		break;
	}

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CMFCApplicationDlg::SetTimerStressPos(int iNewPos)
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
