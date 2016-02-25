#include "stdafx.h"
#include "resource.h"
#include "MainDlg.h"
#include "..\IWorkflowWebImpl.h"
#include "SimulatorService.h"

LRESULT CMainDlg::OnBnClickedRunping(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CComObject < CSimulatorPing >* pTask = NULL;
	HRESULT hr = CComObject < CSimulatorPing >::CreateInstance(&pTask);
	pTask->Init();

	return 0;
}
