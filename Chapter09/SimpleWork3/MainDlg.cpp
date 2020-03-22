// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MainDlg.h"

void CMainDlg::OnCallback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work) {
	auto dlg = (CMainDlg*)context;

	// post message indicating start
	dlg->PostMessage(WM_APP + 1, ::GetCurrentThreadId());

	// simulate work...
	::Sleep(10 * (::GetTickCount() & 0xff));

	// post message indicating end
	dlg->PostMessage(WM_APP + 2, ::GetCurrentThreadId());
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// center the dialog on the screen
	CenterWindow();

	m_List.Attach(GetDlgItem(IDC_WORKITEMS));

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, 0, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, 0, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	m_ThreadPool = ::CreateThreadpool(nullptr);
	if (!m_ThreadPool) {
		AtlMessageBox(*this, L"Failed to create thread pool", IDR_MAINFRAME, MB_ICONERROR);
		EndDialog(IDCANCEL);
		return 0;
	}
	::SetThreadpoolThreadMaximum(m_ThreadPool, 256);
	::SetThreadpoolThreadMinimum(m_ThreadPool, ::GetActiveProcessorCount(ALL_PROCESSOR_GROUPS));
	TP_POOL_STACK_INFORMATION stackInfo;
	stackInfo.StackCommit = 1 << 13;		// 8 KB
	stackInfo.StackReserve = 1 << 14;		// 16 KB
	::SetThreadpoolStackInformation(m_ThreadPool, &stackInfo);
	
	::InitializeThreadpoolEnvironment(&m_CbEnv);
	::SetThreadpoolCallbackPool(&m_CbEnv, m_ThreadPool);

	m_Work = ::CreateThreadpoolWork(OnCallback, this, &m_CbEnv);
	if (!m_Work) {
		AtlMessageBox(*this, L"Failed to create thread pool work", IDR_MAINFRAME, MB_ICONERROR);
		EndDialog(IDCANCEL);
		return 0;
	}

	SetTimer(1, 2000, nullptr);

	return TRUE;
}

LRESULT CMainDlg::OnTimer(UINT, WPARAM id, LPARAM, BOOL&) {
	if (id != 1)
		return 0;

	// enumerate processes
	auto hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
		return 0;

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);

	::Process32First(hSnapshot, &pe);

	auto pid = ::GetCurrentProcessId();
	ULONG threads = 0;
	while (::Process32Next(hSnapshot, &pe)) {
		if (pe.th32ProcessID == pid) {
			threads = pe.cntThreads;
			break;
		}
	}
	::CloseHandle(hSnapshot);

	CString text;
	text.Format(L"Threads: %u\n", threads);
	SetDlgItemText(IDC_THREADS, text);

	return 0;
}

LRESULT CMainDlg::OnDestroy(UINT, WPARAM, LPARAM, BOOL&) {
	::CloseThreadpool(m_ThreadPool);
	::CloseThreadpoolWork(m_Work);

	return 0;
}

LRESULT CMainDlg::OnCallbackStart(UINT, WPARAM wParam, LPARAM, BOOL&) {
	CString text;
	text.Format(L"Started on thread %d", wParam);
	m_List.AddString(text);

	return 0;
}

LRESULT CMainDlg::OnCallbackEnd(UINT, WPARAM wParam, LPARAM, BOOL&) {
	CString text;
	text.Format(L">>ended on thread %d", wParam);
	m_List.AddString(text);

	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnSubmitWorkItem(WORD, WORD wID, HWND, BOOL&) {
	::SubmitThreadpoolWork(m_Work);

	return 0;
}

LRESULT CMainDlg::OnSubmit10WorkItems(WORD, WORD, HWND, BOOL&) {
	for (int i = 0; i < 10; i++) {
		::SubmitThreadpoolWork(m_Work);
	}
	return 0;
}

LRESULT CMainDlg::OnClear(WORD, WORD wID, HWND, BOOL&) {
	m_List.ResetContent();

	return 0;
}
