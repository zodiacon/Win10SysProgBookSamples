// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MainDlg.h"
#include <wil\resource.h>

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	return TRUE;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnRedirect(WORD, WORD wID, HWND, BOOL&) {
	wil::unique_handle hRead, hWrite;
	if (!::CreatePipe(hRead.addressof(), hWrite.addressof(), nullptr, 0))
		return Error(L"Failed to create pipe");

	// make write handle inheritable
	::SetHandleInformation(hWrite.get(), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

	if (!CreateOtherProcess(hWrite.get()))
		return Error(L"Failed to create process");

	// local write handle not needed anymore
	hWrite.reset();

	char buffer[1 << 12] = { 0 };
	DWORD bytes;
	CEdit edit(GetDlgItem(IDC_TEXT));
	ATLASSERT(edit);

	while (::ReadFile(hRead.get(), buffer, sizeof(buffer), &bytes, nullptr) && bytes > 0) {
		CString text;
		edit.GetWindowText(text);
		text += CString(buffer);
		edit.SetWindowText(text);
		::memset(buffer, 0, sizeof(buffer));
	}

	return 0;
}

LRESULT CMainDlg::OnClear(WORD, WORD wID, HWND, BOOL&) {
	GetDlgItem(IDC_TEXT).SetWindowText(L"");

	return 0;
}

LRESULT CMainDlg::Error(PCWSTR text) const {
	CString etext;
	etext.Format(L"%s (%d)", text, ::GetLastError());
	::MessageBox(*this, etext, L"Simple Redirect", MB_ICONERROR);
	return 0;
}

bool CMainDlg::CreateOtherProcess(HANDLE hOutput) {
	PROCESS_INFORMATION pi;
	STARTUPINFO si = { sizeof(si) };
	si.hStdOutput = hOutput;
	si.dwFlags = STARTF_USESTDHANDLES;

	WCHAR path[MAX_PATH];
	::GetModuleFileName(nullptr, path, _countof(path));
	*::wcsrchr(path, L'\\') = L'\0';
	::wcscat_s(path, L"\\EnumDevices.exe");

	BOOL created = ::CreateProcess(nullptr, path, nullptr, nullptr, TRUE, 
		CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
	if (created) {
		::CloseHandle(pi.hProcess);
		::CloseHandle(pi.hThread);
	}

	return created;
}
