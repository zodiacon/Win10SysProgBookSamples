// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include <algorithm>
#include "MainDlg.h"
#include <string>
#include "..\ThreadingHelpers\AutoCriticalSection.h"
#include "..\ThreadingHelpers\AutoReaderWriterLock.h"

CMainDlg* CMainDlg::m_pThis;

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	m_pThis = this;

	m_hAbortEvent.reset(::CreateEvent(nullptr, TRUE, FALSE, nullptr));
	ATLASSERT(m_hAbortEvent);

	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	m_ThreadList.Attach(GetDlgItem(IDC_LIST_THREADS));

	SetDlgItemInt(IDC_PRODUCERS, 2);
	SetDlgItemInt(IDC_CONSUMERS, 2);

	// init consumer list view

	m_ThreadList.InsertColumn(0, L"#", 0, 30);
	m_ThreadList.InsertColumn(1, L"ID", LVCFMT_RIGHT, 60);
	m_ThreadList.InsertColumn(2, L"Items Processed", LVCFMT_RIGHT, 100);
	m_ThreadList.InsertColumn(3, L"Primes", LVCFMT_RIGHT, 60);
	m_ThreadList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER);

	return TRUE;
}

LRESULT CMainDlg::OnTimer(UINT, WPARAM id, LPARAM, BOOL&) {
	if (id == 1) {
		size_t size;
		{
			AutoCriticalSection locker(m_QueueLock);
			size = m_Queue.size();
		}
		SetDlgItemInt(IDC_QUEUE_SIZE, (unsigned)size, FALSE);
	}
	return 0;
}

LRESULT CMainDlg::OnRun(WORD, WORD, HWND, BOOL&) {
	Run();

	return 0;
}

LRESULT CMainDlg::OnStop(WORD, WORD, HWND, BOOL&) {
	Stop();

	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Stop();
	EndDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnUpdateThread(UINT, WPARAM index, LPARAM, BOOL&) {
	auto& data = m_ConsumerThreads[index];
	int n = (int)index;

	CString text;
	text.Format(L"%u", ::InterlockedAdd((LONG*)&data.ItemsProcessed, 0));
	m_ThreadList.SetItemText(n, 2, text);
	text.Format(L"%u", ::InterlockedAdd((LONG*)&data.Primes, 0));
	m_ThreadList.SetItemText(n, 3, text);

	return 0;
}

void CMainDlg::DisplayError(PCWSTR text) {
	AtlMessageBox(nullptr, text, L"Queue Demo", MB_ICONERROR);
}

bool CMainDlg::IsPrime(unsigned n) {
	auto limit = (unsigned)::sqrt(n);
	for (unsigned i = 2; i <= limit; i++)
		if (n % i == 0)
			return false;

	return true;
}

void CMainDlg::Stop() {
	// signal threads to abort

	::SetEvent(m_hAbortEvent.get());
	::WakeAllConditionVariable(&m_QueueCondVar);

	// update UI

	GetDlgItem(IDC_RUN).EnableWindow(TRUE);
	GetDlgItem(IDC_STOP).EnableWindow(FALSE);

}

void CMainDlg::Run() {
	int consumers = GetDlgItemInt(IDC_CONSUMERS);
	if (consumers < 1 || consumers > 64) {
		DisplayError(L"Consumer threads must be between 1 and 64");
		return;
	}
	int producers = GetDlgItemInt(IDC_PRODUCERS);
	if (producers < 1 || producers > 64) {
		DisplayError(L"Producer threads must be between 1 and 64");
		return;
	}

	// create consumer threads

	bool abort = false;
	::ResetEvent(m_hAbortEvent.get());
	::InitializeConditionVariable(&m_QueueCondVar);
	m_ThreadList.DeleteAllItems();

	m_ConsumerThreads.clear();
	m_ConsumerThreads.reserve(consumers);

	for (int i = 0; i < consumers; i++) {
		ConsumerThreadData data;
		data.hThread.reset(::CreateThread(nullptr, 0, [](auto p) {
			return m_pThis->ConsumerThread(PtrToLong(p));
			}, LongToPtr(i), 0, nullptr));
		if (!data.hThread) {
			abort = true;
			break;
		}
		m_ConsumerThreads.push_back(std::move(data));
	}
	if (abort) {
		::SetEvent(m_hAbortEvent.get());
		return;
	}

	// create producer threads

	m_ProducerThreads.clear();
	m_ProducerThreads.reserve(producers);

	for (int i = 0; i < producers; i++) {
		wil::unique_handle hThread(::CreateThread(nullptr, 0, [](auto p) {
			return m_pThis->ProducerThread();
			}, this, 0, nullptr));
		if (!hThread) {
			DisplayError(L"Failed to create producer thread. Aborting");
			abort = true;
			break;
		}
	}

	if (abort) {
		::SetEvent(m_hAbortEvent.get());
		return;
	}

	// update UI

	CString text;
	for (int i = 0; i < (int)m_ConsumerThreads.size(); i++) {
		const auto& t = m_ConsumerThreads[i];
		text.Format(L"%2d", i);
		int n = m_ThreadList.InsertItem(i, text);
		m_ThreadList.SetItemText(n, 1, std::to_wstring(::GetThreadId(t.hThread.get())).c_str());
	}

	GetDlgItem(IDC_RUN).EnableWindow(FALSE);
	GetDlgItem(IDC_STOP).EnableWindow(TRUE);

	SetTimer(1, 500, nullptr);
}

DWORD CMainDlg::ProducerThread() {
	for (;;) {
		if (::WaitForSingleObject(m_hAbortEvent.get(), 0) == WAIT_OBJECT_0)
			break;
		WorkItem item;
		item.IsPrime = false;
		LARGE_INTEGER li;
		::QueryPerformanceCounter(&li);
		item.Data = li.LowPart;
		{
			AutoCriticalSection locker(m_QueueLock);
			m_Queue.push(item);
		}
		::WakeConditionVariable(&m_QueueCondVar);

		// sleep a little bit from time to time
		if ((item.Data & 0x7f) == 0)
			::Sleep(1);
	}
	return 0;
}

DWORD CMainDlg::ConsumerThread(int index) {
	auto& data = m_ConsumerThreads[index];
	auto tick = ::GetTickCount64();

	for (;;) {
		WorkItem value;
		{
			bool abort = false;
			AutoCriticalSection locker(m_QueueLock);
			while (m_Queue.empty()) {
				if (::WaitForSingleObject(m_hAbortEvent.get(), 0) == WAIT_OBJECT_0) {
					abort = true;
					break;
				}
				::SleepConditionVariableCS(&m_QueueCondVar, &m_QueueLock, INFINITE);
			}
			if (abort)
				break;

			ATLASSERT(!m_Queue.empty());
			value = m_Queue.front();
			m_Queue.pop();
		}

		{
			// do the actual work

			bool isPrime = IsPrime(value.Data);

			if (isPrime) {
				value.IsPrime = true;
				::InterlockedIncrement(&data.Primes);
			}
			::InterlockedIncrement(&data.ItemsProcessed);
		}
		auto current = ::GetTickCount64();
		if (current - tick > 600) {
			PostMessage(WM_UPDATE_THREAD, index);
			tick = current;
		}
	}

	PostMessage(WM_UPDATE_THREAD, index);

	return 0;
}
