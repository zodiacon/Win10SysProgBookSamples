// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CMainDlg : 
	public CDialogImpl<CMainDlg>,
	public CMessageFilter {
public:
	enum { IDD = IDD_MAINDLG };

	const UINT WM_ERROR = WM_APP + 1;
	const UINT WM_PROGRESS_START = WM_APP + 2;
	const UINT WM_PROGRESS = WM_APP + 3;
	const UINT WM_DONE = WM_APP + 4;

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_PROGRESS, OnProgress)
		MESSAGE_HANDLER(WM_PROGRESS_START, OnProgressStart)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_GO, OnGo)
		COMMAND_ID_HANDLER(IDC_ADDFILES, OnAddFiles)
		COMMAND_ID_HANDLER(IDC_ADDDIR, OnAddDirectory)
		COMMAND_ID_HANDLER(IDC_SETDEST, OnSetDestination)
		COMMAND_ID_HANDLER(ID_EDIT_SELECT_ALL, OnSelectAll)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_REMOVE, OnRemove)
		NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_DONE, OnDone)
	END_MSG_MAP()

	BOOL PreTranslateMessage(MSG* pMsg);

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnGo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAddFiles(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAddDirectory(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSetDestination(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSelectAll(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnRemove(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnProgress(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnProgressStart(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDone(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

private:
	enum class Type {
		File,
		Folder
	};

	enum class Key {
		Read,
		Write
	};

	struct FileData {
		CString Src;
		CString Dst;
		wil::unique_handle hDst, hSrc;
	};

	struct IOData : OVERLAPPED {
		HANDLE hSrc, hDst;
		std::unique_ptr<BYTE[]> Buffer;
		ULONGLONG Size;
	};

	void UpdateButtons();
	DWORD WorkerThread();
	static CString FormatSize(LONGLONG size);

private:
	HACCEL m_hAccel{ nullptr };
	CListViewCtrl m_List;
	CProgressBarCtrl m_Progress;
	int m_Destinations{ 0 };
	std::vector<FileData> m_Data;
	bool m_Running{ false };
};
