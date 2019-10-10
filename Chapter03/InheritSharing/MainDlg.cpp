#include "..\..\Chapter04\JobMon\MainDlg.h"
#include "..\..\Chapter04\JobMon\MainDlg.h"
// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "MainDlg.h"

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	int count;
	PWSTR* args = ::CommandLineToArgvW(::GetCommandLine(), &count);
	if (count == 1) {
		// "master" instance
		m_hSharedMem.reset(::CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 1 << 16, nullptr));
	}
	else {
		// first "real" argument is inherited handle value
		m_hSharedMem.reset((HANDLE)(ULONG_PTR)::_wtoi(args[1]));
	}
	::LocalFree(args);

	ATLASSERT(m_hSharedMem);

	return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnCreateProcess(WORD, WORD, HWND, BOOL &) {
	// make sure handle is inheritable
	::SetHandleInformation(m_hSharedMem.get(), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;

	// build command line
	WCHAR path[MAX_PATH];
	::GetModuleFileName(nullptr, path, MAX_PATH);
	WCHAR handle[16];
	::_itow_s((int)(ULONG_PTR)m_hSharedMem.get(), handle, 10);
	::wcscat_s(path, L" ");
	::wcscat_s(path, handle);

	// now create the process

	if (::CreateProcess(nullptr, path, nullptr, nullptr, TRUE,
		0, nullptr, nullptr, &si, &pi)) {
		// close unneeded handles
		::CloseHandle(pi.hProcess);
		::CloseHandle(pi.hThread);
	}
	else {
		MessageBox(L"Failed to create new process", L"Inherit Sharing");
	}

	return 0;
}

LRESULT CMainDlg::OnRead(WORD, WORD, HWND, BOOL &) {
	void* buffer = ::MapViewOfFile(m_hSharedMem.get(), FILE_MAP_READ, 0, 0, 0);
	if (!buffer) {
		AtlMessageBox(m_hWnd, L"Failed to map memory", IDR_MAINFRAME);
		return 0;
	}

	SetDlgItemText(IDC_TEXT, (PCWSTR)buffer);
	::UnmapViewOfFile(buffer);

	return 0;
}

LRESULT CMainDlg::OnWrite(WORD, WORD, HWND, BOOL &) {
	void* buffer = ::MapViewOfFile(m_hSharedMem.get(), FILE_MAP_WRITE, 0, 0, 0);
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
