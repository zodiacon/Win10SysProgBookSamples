// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CMainDlg : public CDialogImpl<CMainDlg> {
public:
	enum { IDD = IDD_MAINDLG };
	enum { WM_DONE = WM_APP + 1 };

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DONE, OnDone)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_HANDLER(IDC_RUN, BN_CLICKED, OnRun)
	END_MSG_MAP()

	enum class Algorithm {
		Simple,
		WithInterlocked,
		WithCriticalSection,
		WithMutex
	};

	DWORD DoCount();
	void DoSimpleCount();
	void DoInterlockedCount();
	void DoCriticalSectionCount();
	void DoMutexCount();

	DWORD IncInterlockedThread();
	DWORD IncSimpleThread();
	DWORD IncCriticalSectionThread();
	DWORD IncMutexThread();

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRun(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDone(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

private:
	CComboBox m_AlgorithmCB;
	int m_Threads;
	int m_Loops;
	Algorithm m_Algorithm;
	volatile int m_Count;
	ULONGLONG m_Time;
	CRITICAL_SECTION m_CritSection;
	HANDLE m_hMutex;
};

