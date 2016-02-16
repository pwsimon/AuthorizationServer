// MFCApplicationDlg.h : header file
//

#pragma once

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
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedRunping();

	DECLARE_MESSAGE_MAP()
};
