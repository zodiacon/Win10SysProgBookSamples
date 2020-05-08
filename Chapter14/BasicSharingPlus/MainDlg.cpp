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

	::SHAutoComplete(GetDlgItem(IDC_FILENAME), SHACF_FILESYS_ONLY);

	// check if file mapping object exists
	m_hSharedMem = ::OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, L"MySharedMemory");
	if (m_hSharedMem)
		EnableUI();

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

LRESULT CMainDlg::OnCreate(WORD, WORD, HWND, BOOL&) {
	CString filename;
	GetDlgItemText(IDC_FILENAME, filename);
	HANDLE hFile = INVALID_HANDLE_VALUE;
	if (!filename.IsEmpty()) {
		hFile = ::CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0,
			nullptr, OPEN_ALWAYS, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE) {
			AtlMessageBox(*this, L"Failed to create/open file", IDR_MAINFRAME, MB_ICONERROR);
			return 0;
		}
	}

	m_hSharedMem = ::CreateFileMapping(hFile, nullptr, PAGE_READWRITE, 0, 1 << 12, L"MySharedMemory");
	if (!m_hSharedMem) {
		AtlMessageBox(m_hWnd, L"Failed to create shared memory", IDR_MAINFRAME, MB_ICONERROR);
		EndDialog(IDCANCEL);
	}

	if (hFile != INVALID_HANDLE_VALUE)
		::CloseHandle(hFile);

	EnableUI();

	return 0;
}

void CMainDlg::EnableUI() {
	((CEdit&)GetDlgItem(IDC_FILENAME)).SetReadOnly(TRUE);
	GetDlgItem(IDC_CREATE).EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT).EnableWindow();
	GetDlgItem(IDC_WRITE).EnableWindow();
	GetDlgItem(IDC_READ).EnableWindow();

}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if (m_hSharedMem)
		::CloseHandle(m_hSharedMem);
	return 0;
}


