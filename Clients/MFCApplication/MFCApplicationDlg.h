// MFCApplicationDlg.h : header file
//

#pragma once
#include "afxcmn.h"

// CMFCApplicationDlg dialog
class CMFCApplicationDlg : public CDialogEx
{
// Construction
public:
	CMFCApplicationDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_MFCAPPLICATION_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	CSliderCtrl m_sldrStressTimer;
	int m_TimerStressPos; // current value;
	HICON m_hIcon;
	typedef std::list < CCmdTarget* > LIST_CMDS;
	LIST_CMDS m_lstCmd;

	CTypedPtrList < CObList, CCmdTarget* > m_tpl;
	void SetTimerStressPos(int iNewPos);

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedRunping();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	DECLARE_MESSAGE_MAP()
};
