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

	// create the boundary descriptor
	m_hBD = ::CreateBoundaryDescriptor(L"MyDescriptor", 0);
	if (!m_hBD)
		return ReportError(L"Failed to create boundary descriptor");

	BYTE sid[SECURITY_MAX_SID_SIZE];
	auto psid = reinterpret_cast<PSID>(sid);
	DWORD sidLen;
	if (!::CreateWellKnownSid(WinBuiltinUsersSid, nullptr, psid, &sidLen))
		return ReportError(L"Failed to create SID");

	if (!::AddSIDToBoundaryDescriptor(&m_hBD, psid))
		return ReportError(L"Failed to add SID to Boundary Descriptor");

	// create the private namespace
	m_hNamespace = ::CreatePrivateNamespace(nullptr, m_hBD, L"MyNamespace");
	if (!m_hNamespace) { // maybe created already?
		m_hNamespace = ::OpenPrivateNamespace(m_hBD, L"MyNamespace");
		if (!m_hNamespace)
			return ReportError(L"Failed to create/open private namespace");
	}

	m_hSharedMem.reset(::CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 1 << 12, L"MyNamespace\\MySharedMem"));
	if (!m_hSharedMem)
		return ReportError(L"Failed to create shared memory");

	return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT, WPARAM, LPARAM, BOOL &) {
	if (m_hNamespace)
		::ClosePrivateNamespace(m_hNamespace, 0);
	if (m_hBD)
		::DeleteBoundaryDescriptor(m_hBD);

	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}

LRESULT CMainDlg::ReportError(PCWSTR text) {
	CString msg;
	msg.Format(L"%s (error: %d)", text, ::GetLastError());
	MessageBox(msg, L"Private Sharing");
	EndDialog(IDCANCEL);
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
