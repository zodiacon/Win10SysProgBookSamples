// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "..\ThreadingHelpers\CriticalSection.h"
#include "..\ThreadingHelpers\ReaderWriterLock.h"

class CMainDlg : public CDialogImpl<CMainDlg> {
public:
	enum { IDD = IDD_MAINDLG };
	
	const DWORD WM_UPDATE_THREAD = WM_APP + 1;

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_UPDATE_THREAD, OnUpdateThread)
		COMMAND_ID_HANDLER(IDC_RUN, OnRun)
		COMMAND_ID_HANDLER(IDC_STOP, OnStop)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRun(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnUpdateThread(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	struct ConsumerThreadData {
		unsigned ItemsProcessed{ 0 };
		unsigned Primes{ 0 };
		wil::unique_handle hThread;
	};

	struct WorkItem {
		unsigned Data;
		bool IsPrime;
	};

	static void DisplayError(PCWSTR text);
	static bool IsPrime(unsigned n);

	void Stop();
	void Run();
	DWORD ProducerThread();
	DWORD ConsumerThread(int index);

	static CMainDlg* m_pThis;
	CListViewCtrl m_ThreadList;

	std::queue<WorkItem> m_Queue;
	CriticalSection m_QueueLock;
	CONDITION_VARIABLE m_QueueCondVar;
	std::vector<wil::unique_handle> m_ProducerThreads;
	std::vector<ConsumerThreadData> m_ConsumerThreads;
	wil::unique_handle m_hAbortEvent;
};
