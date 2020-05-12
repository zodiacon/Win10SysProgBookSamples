// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MainDlg.h"

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	m_hSharedMem = ::CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 1 << 12, L"MySharedMemory");
	if (!m_hSharedMem) {
		AtlMessageBox(m_hWnd, L"Failed to create/open shared memory", IDR_MAINFRAME);
		EndDialog(IDCANCEL);
	}

	return TRUE;
}

LRESULT CMainDlg::OnRead(WORD, WORD, HWND, BOOL &) {
	void* buffer = ::MapViewOfFile(m_hSharedMem, FILE_MAP_READ, 0, 0, 0);
	if (!buffer) {
		AtlMessageBox(m_hWnd, L"Failed to map memory", IDR_MAINFRAME);
		return 0;
	}

	SetDlgItemText(IDC_TEXT, (PCWSTR)buffer);
	::UnmapViewOfFile(buffer);

	return 0;
}

LRESULT CMainDlg::OnWrite(WORD, WORD, HWND, BOOL &) {
	void* buffer = ::MapViewOfFile(m_hSharedMem, FILE_MAP_WRITE, 0, 0, 0);
	if (!buffer) {
		AtlMessageBox(m_hWnd, L"Failed to map memory", IDR_MAINFRAME);
		return 0;
	}

	CString text;
	GetDlgItemText(IDC_TEXT, text);
	::wcscpy_s((PWSTR)buffer, text.GetLength() + 1, text);

	::UnmapViewOfFile(buffer);

	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if (m_hSharedMem)
		::CloseHandle(m_hSharedMem);
	return 0;
}


