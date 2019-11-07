// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CMainDlg : public CDialogImpl<CMainDlg> {
public:
	enum { IDD = IDD_MAINDLG };

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		NOTIFY_HANDLER(IDC_PROCESSES_IN_JOB, LVN_GETDISPINFO, OnGetDisplayInfo)
		COMMAND_ID_HANDLER(IDC_CREATEJOB, OnCreateJob)
		COMMAND_ID_HANDLER(IDC_OPEN_JOB, OnOpenJob)
		COMMAND_ID_HANDLER(IDC_ADD_EXISTING_PROCESS, OnAddExistingProcess)
		COMMAND_ID_HANDLER(IDC_CREATE_PROCESS, OnCreateProcess)
		COMMAND_ID_HANDLER(IDC_TERMINATE, OnTerminateJob)
		COMMAND_ID_HANDLER(IDC_CLOSE_HANDLE, OnCloseHandle)
		COMMAND_ID_HANDLER(IDC_OPEN_JOB, OnOpenJob)
		COMMAND_ID_HANDLER(IDC_BROWSE, OnBrowse)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_REFRESH_PROCESSES, OnRefresh)
		COMMAND_ID_HANDLER(IDC_SET_LIMIT, OnSetLimit)
		COMMAND_ID_HANDLER(IDC_REMOVE_LIMIT, OnRemoveLimit)
		COMMAND_ID_HANDLER(IDC_REFRESH_LIMITS, OnRefreshLimits)
		COMMAND_ID_HANDLER(IDC_BIND_IO_COMPLETION, OnBindToIoCompletion)
		COMMAND_HANDLER(IDC_COMBO_LIMITS, CBN_SELCHANGE, OnLimitSelectionChanged)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	LRESULT OnLimitSelectionChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnGetDisplayInfo(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCreateJob(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOpenJob(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAddExistingProcess(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCreateProcess(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnTerminateJob(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRefresh(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBrowse(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSetLimit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRemoveLimit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRefreshLimits(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBindToIoCompletion(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCloseHandle(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	struct ColumnInfo {
		PCWSTR Name;
		int Width;
		int Format = LVCFMT_LEFT;
	};

	void InitControls();
	void BuildLimitList();
	void AddColumnsToListView(CListViewCtrl& lv, const ColumnInfo* columns, int count);
	void DisplayError(PCWSTR message);
	void AddLog(PCWSTR message);
	void AddLog(const std::wstring& message);
	void UpdateJob();
	void BeginWriteInfo(UINT groupId);
	void RefreshSystemProcessList();
	void UpdateUIForNewJob(BOOL reset = FALSE);
	void UpdateJobLimits();

	template<typename T>
	void AddLimit(PCWSTR name, const T& value) {
		m_Limits.AddString((name + std::to_wstring(value)).c_str());
	}
	void AddLimit(PCWSTR name);
	void AddLimit(const std::wstring& limit);

	template<typename T>
	void AddDataItem(PCWSTR property, const T& value) {
		AddDataItem(property, CString(std::to_wstring(value).c_str()));
	}
	void AddDataItem(PCWSTR property, const LARGE_INTEGER& li);
	void AddDataItem(PCWSTR property, const CTimeSpan& ts);
	void AddDataItem(PCWSTR property, const CString& value);
	static CString GetProcessName(DWORD pid);
	static CString PriorityClassToString(int priority);
	template<typename T>
	static T ParseNumber(const CString& text) {
		if (text.Left(2) == L"0x")
			return (T)wcstoll(text.Mid(2), nullptr, 16);
		return (T)_wtoll(text);
	}

	DWORD DoMonitorJob();

	struct ProcessInfo {
		CString Name;
		DWORD Id;
	};
	enum class JobLimitType {
		ActiveProcesses,
		BreakawayOk,
		SilentBreakawayOk,
		JobMemory,
		ProcessMemory,
		KillOnJobClose,
		PriorityClass,
		ProcessTime,
		SchedulingClass,
		MaxWorkingSet,
		JobTime,
		Affinity,
		CpuRateControl,
		DieOnUnhandledException,
		UserInterface_Desktop			= 0x100 | JOB_OBJECT_UILIMIT_DESKTOP,
		UserInterface_ReadClipboard		= 0x100 | JOB_OBJECT_UILIMIT_READCLIPBOARD,
		UserInterface_WriteClipboard	= 0x100 | JOB_OBJECT_UILIMIT_WRITECLIPBOARD,
		UserInterface_All				= 0x100 | JOB_OBJECT_UILIMIT_ALL,
		UserInterface_GlobalAtoms		= 0x100 | JOB_OBJECT_UILIMIT_GLOBALATOMS,
		UserInterface_ExitWindows		= 0x100 | JOB_OBJECT_UILIMIT_EXITWINDOWS,
		UserInterface_DisplaySettings	= 0x100 | JOB_OBJECT_UILIMIT_DISPLAYSETTINGS,
		UserInterface_SystemParameters	= 0x100 | JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS,
		UserInterface_Handles			= 0x100 | JOB_OBJECT_UILIMIT_HANDLES,
	};

private:
	std::vector<ProcessInfo> m_ProcessList;
	CListViewCtrl m_Log, m_Processes;
	CComboBox m_ProcessCombo;
	CListBox m_Limits;
	CComboBox m_LimitsCombo;
	CComboBox m_ValueCombo, m_UnitsCombo;
	CEdit m_EditValue;
	CRect m_InfoRectProperty, m_InfoRectValue;
	int m_Height;
	wil::unique_handle m_hJob, m_hCompletionPort;
};
