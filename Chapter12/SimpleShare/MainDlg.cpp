// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MainDlg.h"

#pragma data_seg("shared")
int SharedValue = 0;
#pragma data_seg()

#pragma comment(linker, "/section:shared,RWS")

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);
	
	CString text;
	text.Format(L"0x%p", &SharedValue);
	SetDlgItemText(IDC_ADDRESS, text);

	SetTimer(1, 1000, nullptr);

	return TRUE;
}

LRESULT CMainDlg::OnTimer(UINT, WPARAM id, LPARAM, BOOL&) {
	if (id == 1)
		SetDlgItemInt(IDC_VALUE, SharedValue);

	return 0;
}

LRESULT CMainDlg::OnIncrement(WORD, WORD wID, HWND, BOOL&) {
	SharedValue++;	// InterlockedIncrement

	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnReset(WORD, WORD wID, HWND, BOOL&) {
	SharedValue = 0;

	return 0;
}
