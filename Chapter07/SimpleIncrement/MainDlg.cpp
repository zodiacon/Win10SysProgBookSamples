#include "..\QueueDemo\MainDlg.h"
// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MainDlg.h"

DWORD CMainDlg::DoCount() {
	auto now = ::GetTickCount64();
	switch (m_Algorithm) {
		case Algorithm::Simple: 
			DoSimpleCount();
			break;

		case Algorithm::WithInterlocked:
			DoInterlockedCount();
			break;

		case Algorithm::WithCriticalSection: 
			DoCriticalSectionCount();
			break;

		case Algorithm::WithMutex:
			DoMutexCount();
			break;
	}
	m_Time = ::GetTickCount64() - now;
	PostMessage(WM_DONE);

	return 0;
}

void CMainDlg::DoSimpleCount() {
	auto handles = std::make_unique<HANDLE[]>(m_Threads);
	for (int i = 0; i < m_Threads; i++) {
		handles[i] = ::CreateThread(nullptr, 0, [](auto param) {
			return ((CMainDlg*)param)->IncSimpleThread();
			}, this, 0, nullptr);
	}
	::WaitForMultipleObjects(m_Threads, handles.get(), TRUE, INFINITE);
	for (int i = 0; i < m_Threads; i++)
		::CloseHandle(handles[i]);
}

void CMainDlg::DoInterlockedCount() {
	auto handles = std::make_unique<HANDLE[]>(m_Threads);
	for (int i = 0; i < m_Threads; i++) {
		handles[i] = ::CreateThread(nullptr, 0, [](auto param) {
			return ((CMainDlg*)param)->IncInterlockedThread();
			}, this, 0, nullptr);
	}
	::WaitForMultipleObjects(m_Threads, handles.get(), TRUE, INFINITE);
	for (int i = 0; i < m_Threads; i++)
		::CloseHandle(handles[i]);
}

void CMainDlg::DoCriticalSectionCount() {
	auto handles = std::make_unique<HANDLE[]>(m_Threads);
	::InitializeCriticalSection(&m_CritSection);

	for (int i = 0; i < m_Threads; i++) {
		handles[i] = ::CreateThread(nullptr, 0, [](auto param) {
			return ((CMainDlg*)param)->IncCriticalSectionThread();
			}, this, 0, nullptr);
	}
	::WaitForMultipleObjects(m_Threads, handles.get(), TRUE, INFINITE);
	for (int i = 0; i < m_Threads; i++)
		::CloseHandle(handles[i]);
	::DeleteCriticalSection(&m_CritSection);
}

void CMainDlg::DoMutexCount() {
	auto handles = std::make_unique<HANDLE[]>(m_Threads);
	m_hMutex = ::CreateMutex(nullptr, FALSE, nullptr);

	for (int i = 0; i < m_Threads; i++) {
		handles[i] = ::CreateThread(nullptr, 0, [](auto param) {
			return ((CMainDlg*)param)->IncMutexThread();
			}, this, 0, nullptr);
	}
	::WaitForMultipleObjects(m_Threads, handles.get(), TRUE, INFINITE);
	for (int i = 0; i < m_Threads; i++)
		::CloseHandle(handles[i]);
	::CloseHandle(m_hMutex);
}

DWORD CMainDlg::IncInterlockedThread() {
	for (int i = 0; i < m_Loops; i++)
		::InterlockedIncrement((unsigned*)&m_Count);
	return 0;
}

DWORD CMainDlg::IncSimpleThread() {
	for (int i = 0; i < m_Loops; i++)
		m_Count++;
	return 0;
}

DWORD CMainDlg::IncCriticalSectionThread() {
	for (int i = 0; i < m_Loops; i++) {
		::EnterCriticalSection(&m_CritSection);
		m_Count++;
		::LeaveCriticalSection(&m_CritSection);
	}
	return 0;
}

DWORD CMainDlg::IncMutexThread() {
	for (int i = 0; i < m_Loops; i++) {
		::WaitForSingleObject(m_hMutex, INFINITE);
		m_Count++;
		::ReleaseMutex(m_hMutex);
	}
	return 0;
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	m_AlgorithmCB.Attach(GetDlgItem(IDC_ALGORITHM));
	m_AlgorithmCB.SetItemData(m_AlgorithmCB.AddString(L"None"), (DWORD_PTR)Algorithm::Simple);
	m_AlgorithmCB.SetItemData(m_AlgorithmCB.AddString(L"Interlocked"), (DWORD_PTR)Algorithm::WithInterlocked);
	m_AlgorithmCB.SetItemData(m_AlgorithmCB.AddString(L"Critical Section"), (DWORD_PTR)Algorithm::WithCriticalSection);
	m_AlgorithmCB.SetItemData(m_AlgorithmCB.AddString(L"Mutex"), (DWORD_PTR)Algorithm::WithMutex);
	m_AlgorithmCB.SetCurSel(0);

	SetDlgItemInt(IDC_THREADS, 4);
	SetDlgItemInt(IDC_LOOPS, 1000000);

	return TRUE;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnRun(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int threads = GetDlgItemInt(IDC_THREADS);
	if (threads < 1 || threads > MAXIMUM_WAIT_OBJECTS) {
		AtlMessageBox(*this, L"Thread count must be between 1 and 64.", IDR_MAINFRAME, MB_ICONERROR);
		return 0;
	}
	int loops = GetDlgItemInt(IDC_LOOPS);
	if (loops < 1000) {
		AtlMessageBox(*this, L"Please use loops equal or greater than 1000.", IDR_MAINFRAME, MB_ICONERROR);
		return 0;
	}
	m_Threads = threads;
	m_Loops = loops;
	m_Algorithm = (Algorithm)m_AlgorithmCB.GetItemData(m_AlgorithmCB.GetCurSel());
	m_Count = 0;
	SetDlgItemInt(IDC_EXPECTED, threads * loops);
	SetDlgItemText(IDC_ELAPSED, L"Working...");
	SetDlgItemInt(IDC_TOTALCOUNT, 0);

	::CloseHandle(::CreateThread(nullptr, 0, [](auto p) {
		return ((CMainDlg*)p)->DoCount();
		}, this, 0, nullptr));

	GetDlgItem(IDC_RUN).EnableWindow(FALSE);

	return 0;
}

LRESULT CMainDlg::OnDone(UINT, WPARAM, LPARAM, BOOL&) {
	SetDlgItemInt(IDC_TOTALCOUNT, m_Count);
	SetDlgItemInt(IDC_ELAPSED, (unsigned)m_Time, FALSE);
	GetDlgItem(IDC_RUN).EnableWindow();

	return 0;
}
