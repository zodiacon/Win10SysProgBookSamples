// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MainDlg.h"

int CMainDlg::FixMemory(void* address, DWORD exceptionCode) {
	if (exceptionCode == EXCEPTION_ACCESS_VIOLATION) {
		::VirtualAlloc(address, CellSize, MEM_COMMIT, PAGE_READWRITE);
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

void* CMainDlg::GetCell(int& x, int& y, bool reportError) const {
	x = GetDlgItemInt(IDC_CELLX);
	y = GetDlgItemInt(IDC_CELLY);
	if (x < 0 || x >= SizeX || y < 0 || y >= SizeY) {
		if(reportError)
			AtlMessageBox(*this, L"Indices out of range", IDR_MAINFRAME, MB_ICONEXCLAMATION);
		return nullptr;
	}
	return (BYTE*)m_Address + CellSize * ((size_t)x + SizeX * y);
}

bool CMainDlg::AllocateRegion() {
	m_Address = ::VirtualAlloc(nullptr, TotalSize, MEM_RESERVE, PAGE_READWRITE);
	if (!m_Address) {
		AtlMessageBox(nullptr, L"Available address space is not large enough", 
			IDR_MAINFRAME, MB_ICONERROR);
		EndDialog(IDCANCEL);
		return false;
	}

	CString addr;
	addr.Format(L"0x%p", m_Address);
	SetDlgItemText(IDC_ADDRESS, addr);
	SetDlgItemText(IDC_CELLADDR, addr);

	return true;
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// center the dialog on the screen
	CenterWindow();

	if (!AllocateRegion())
		return 0;

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	m_Xspin.Attach(GetDlgItem(IDC_CELLXSPIN));
	m_Xspin.SetRange32(0, SizeX - 1);

	m_Yspin.Attach(GetDlgItem(IDC_CELLYSPIN));
	m_Yspin.SetRange32(0, SizeY - 1);

	SetTimer(1, 1000, nullptr);

	return TRUE;
}

LRESULT CMainDlg::OnWrite(WORD /*wNotifyCode*/, WORD, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int x, y;
	auto p = GetCell(x, y);
	if(!p)
		return 0;

	WCHAR text[512];
	GetDlgItemText(IDC_TEXT, text, _countof(text));

	__try {
		::wcscpy_s((WCHAR*)p, CellSize / sizeof(WCHAR), text);
	}
	__except (FixMemory(p, GetExceptionCode())) {
		// nothing to do: this code is never reached
	}

	return 0;
}

LRESULT CMainDlg::OnRead(WORD, WORD, HWND, BOOL&) {
	int x, y;
	auto p = GetCell(x, y);
	if(!p)
		return 0;

	WCHAR text[512];
	__try {
		::wcscpy_s(text, _countof(text), (PCWSTR)p);
		SetDlgItemText(IDC_TEXT, text);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		AtlMessageBox(nullptr, L"Cell memory is not committed", IDR_MAINFRAME, MB_ICONWARNING);
	}
	return 0;
}

LRESULT CMainDlg::OnRelease(WORD, WORD, HWND, BOOL&) {
	int x, y;
	auto p = GetCell(x, y);
	if (p) {
		::VirtualFree(p, CellSize, MEM_DECOMMIT);
	}
	return 0;
}

LRESULT CMainDlg::OnReleaseAll(WORD, WORD wID, HWND, BOOL&) {
	::VirtualFree(m_Address, 0, MEM_RELEASE);
	// allocate a new reserved region
	AllocateRegion();

	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnTimer(UINT, WPARAM id, LPARAM, BOOL&) {
	if (id == 1) {
		MEMORY_BASIC_INFORMATION mbi;
		auto p = (BYTE*)m_Address;
		size_t committed = 0;
		while (p < (BYTE*)m_Address + TotalSize) {
			::VirtualQuery(p, &mbi, sizeof(mbi));
			if (mbi.State == MEM_COMMIT)
				committed += mbi.RegionSize;
			p += mbi.RegionSize;
		}
		CString text;
		text.Format(L"Total: %llu KB Committed: %llu KB", TotalSize >> 10, committed >> 10);
		SetDlgItemText(IDC_STATS, text);
	}
	return 0;
}

LRESULT CMainDlg::OnDestroy(UINT, WPARAM, LPARAM, BOOL&) {
	if (m_Address)
		::VirtualFree(m_Address, 0, MEM_RELEASE);

	return 0;
}

LRESULT CMainDlg::OnCellTextChanged(WORD, WORD wID, HWND, BOOL&) {
	if (m_Address == nullptr)
		return 0;

	int x, y;
	CString text;

	auto p = GetCell(x, y, false);
	if(p) {
		text.Format(L"0x%p", p);
	}
	SetDlgItemText(IDC_CELLADDR, text);
	return 0;
}
