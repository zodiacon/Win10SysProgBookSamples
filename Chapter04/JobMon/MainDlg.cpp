// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MainDlg.h"

namespace {
	long long ToSeconds(long long ticks) {
		return ticks / 10000000LL;
	}
}

LRESULT CMainDlg::OnLimitSelectionChanged(WORD, WORD, HWND, BOOL&) {
	m_ValueCombo.ResetContent();
	m_UnitsCombo.ResetContent();
	m_EditValue.SetWindowText(L"");
	m_EditValue.EnableWindow(TRUE);

	auto type = static_cast<JobLimitType>(m_LimitsCombo.GetItemData(m_LimitsCombo.GetCurSel()));
	switch (type) {
		case JobLimitType::ProcessMemory:
		case JobLimitType::JobMemory:
		case JobLimitType::MaxWorkingSet:
		{
			static struct {
				PCWSTR Name;
				int Factor;
			} units[] = {
//				{ L"KB", 10 },
				{ L"MB", 20 },
				{ L"GB", 30 }
			};
			for (auto& unit : units) {
				int n = m_UnitsCombo.AddString(unit.Name);
				m_UnitsCombo.SetItemData(n, unit.Factor);
			}
			m_UnitsCombo.SetCurSel(0);
			m_EditValue.ShowWindow(SW_SHOW);
			m_ValueCombo.ShowWindow(SW_HIDE);
			break;
		}

		case JobLimitType::ActiveProcesses:
			m_ValueCombo.ShowWindow(SW_HIDE);
			m_EditValue.ShowWindow(SW_SHOW);
			break;

		case JobLimitType::Affinity:
			m_EditValue.ShowWindow(SW_SHOW);
			break;

		case JobLimitType::PriorityClass:
		{
			static struct {
				PCWSTR Name;
				int Value;
			} classes[] = { 
				{ L"Normal", NORMAL_PRIORITY_CLASS },
				{ L"Below Normal", BELOW_NORMAL_PRIORITY_CLASS },
				{ L"Above Normal", ABOVE_NORMAL_PRIORITY_CLASS },
				{ L"High", HIGH_PRIORITY_CLASS },
				{ L"Idle", IDLE_PRIORITY_CLASS },
				{ L"Realtime", REALTIME_PRIORITY_CLASS }
			};
			for (auto& cls : classes) {
				int n = m_ValueCombo.AddString(cls.Name);
				m_ValueCombo.SetItemData(n, cls.Value);
			}
			m_ValueCombo.SetCurSel(0);
			m_EditValue.ShowWindow(SW_HIDE);
			m_ValueCombo.ShowWindow(SW_SHOW);
			break;
		}

		default:
			m_EditValue.ShowWindow(FALSE);
			m_ValueCombo.ShowWindow(SW_HIDE);
			break;
	}

	return 0;
}

LRESULT CMainDlg::OnGetDisplayInfo(int, LPNMHDR hdr, BOOL&) {
	auto lv = reinterpret_cast<NMLVDISPINFO*>(hdr);
	auto& item = lv->item;
	auto& data = m_ProcessList[item.iItem];

	if (item.mask & LVIF_TEXT) {
		switch (lv->item.iSubItem) {
			case 0:	// name
				::StringCchCopy(item.pszText, item.cchTextMax, data.Name);
				break;

			case 1:	// ID
				::StringCchPrintf(item.pszText, item.cchTextMax, L"%d (0x%X)", data.Id, data.Id);
				break;
		}
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

	CMenuHandle menu = GetSystemMenu(FALSE);
	menu.AppendMenuW(MF_SEPARATOR);
	menu.AppendMenuW(MF_BYCOMMAND, ID_APP_ABOUT, L"About Job Monitor...");

	InitControls();

	return TRUE;
}

LRESULT CMainDlg::OnTimer(UINT, WPARAM id, LPARAM, BOOL&) {
	if (id != 1)
		return 0;

	if (m_hJob)
		UpdateJob();
	return 0;
}

void CMainDlg::InitControls() {
	m_Log.Attach(GetDlgItem(IDC_LOG));
	m_Log.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
	m_Processes.Attach(GetDlgItem(IDC_PROCESSES_IN_JOB));
	m_Processes.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
	m_ProcessCombo.Attach(GetDlgItem(IDC_PROCESS_LIST));
	m_Limits.Attach(GetDlgItem(IDC_LIMITS));
	m_LimitsCombo.Attach(GetDlgItem(IDC_COMBO_LIMITS));
	m_ValueCombo.Attach(GetDlgItem(IDC_LIMIT_VALUE));
	m_UnitsCombo.Attach(GetDlgItem(IDC_UNITS));
	m_EditValue.Attach(GetDlgItem(IDC_EDIT_VALUE));

	ColumnInfo logColumns[] = {
		{ L"Time", 70 },
		{ L"Message", 300 }
	};
	AddColumnsToListView(m_Log, logColumns, _countof(logColumns));

	ColumnInfo procColumns[] = {
		{ L"Name", 150 },
		{ L"PID", 100, LVCFMT_RIGHT },
	};
	AddColumnsToListView(m_Processes, procColumns, _countof(procColumns));

	RefreshSystemProcessList();
	BuildLimitList();
}

void CMainDlg::BuildLimitList() {
	struct {
		PCWSTR Name;
		JobLimitType Type;
	} limits[] = {
		{ L"Active Processes", JobLimitType::ActiveProcesses },
		{ L"Breakaway OK", JobLimitType::BreakawayOk },
		{ L"Silent Breakaway OK", JobLimitType::SilentBreakawayOk },
		{ L"Kill on Job Close", JobLimitType::KillOnJobClose },
		{ L"Job Time", JobLimitType::JobTime },
		{ L"Process Time", JobLimitType::ProcessTime },
		{ L"Process Memory", JobLimitType::ProcessMemory },
		{ L"Job Memory", JobLimitType::JobMemory },
		{ L"Scheduling Class", JobLimitType::SchedulingClass },
		{ L"Affinity", JobLimitType::Affinity },
		{ L"CPU Rate Control", JobLimitType::CpuRateControl },
		{ L"Maximum Working Set", JobLimitType::MaxWorkingSet },
		{ L"Priority Class", JobLimitType::PriorityClass },
		{ L"Die on Unhandled Exception", JobLimitType::DieOnUnhandledException },
		{ L"User Interface - Desktop", JobLimitType::UserInterface_Desktop },
		{ L"User Interface - Global Atoms", JobLimitType::UserInterface_GlobalAtoms },
		{ L"User Interface - Read Clipboard", JobLimitType::UserInterface_ReadClipboard },
		{ L"User Interface - Write Clipboard", JobLimitType::UserInterface_WriteClipboard },
		{ L"User Interface - All", JobLimitType::UserInterface_All },
		{ L"User Interface - Display Settings", JobLimitType::UserInterface_DisplaySettings },
		{ L"User Interface - Exit Windows", JobLimitType::UserInterface_ExitWindows },
		{ L"User Interface - System Parameters", JobLimitType::UserInterface_SystemParameters },
		{ L"User Interface - Handles", JobLimitType::UserInterface_Handles },
	};

	for (const auto& limit : limits) {
		int n = m_LimitsCombo.AddString(limit.Name);
		m_LimitsCombo.SetItemData(n, static_cast<DWORD>(limit.Type));
	}
	m_LimitsCombo.SetCurSel(0);
}

void CMainDlg::AddColumnsToListView(CListViewCtrl& lv, const ColumnInfo* columns, int count) {
	for (int i = 0; i < count; i++) {
		auto& col = columns[i];
		lv.InsertColumn(i, col.Name, col.Format, col.Width);
	}
}

void CMainDlg::DisplayError(PCWSTR message) {
	CString text;
	text.Format(L"Error: %s (%d)", message, ::GetLastError());
	MessageBox(text, L"Job Monitor", MB_ICONERROR);
	AddLog(text);
}

void CMainDlg::AddLog(PCWSTR message) {
	auto now = CTime::GetCurrentTime();
	int n = m_Log.InsertItem(m_Log.GetItemCount(), now.Format(L"%X"));
	m_Log.SetItemText(n, 1, message);
	m_Log.EnsureVisible(n, FALSE);
}

void CMainDlg::AddLog(const std::wstring& message) {
	AddLog(message.c_str());
}

void CMainDlg::UpdateJob() {
	if (!m_hJob) {
		BeginWriteInfo(IDC_GROUP_INFO);
		BeginWriteInfo(IDC_GROUP_IO);
		m_Processes.SetItemCount(0);
		return;
	}

	{
		JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION info;
		if (::QueryInformationJobObject(m_hJob.get(), JobObjectBasicAndIoAccountingInformation, &info, sizeof(info), nullptr)) {
			BeginWriteInfo(IDC_GROUP_INFO);
			AddDataItem(L"Active Processes:", info.BasicInfo.ActiveProcesses);
			AddDataItem(L"Total Processes:", info.BasicInfo.TotalProcesses);
			AddDataItem(L"Terminated Processes:", info.BasicInfo.TotalTerminatedProcesses);
			AddDataItem(L"Total User Time:", CTimeSpan(ToSeconds(info.BasicInfo.TotalUserTime.QuadPart)));
			AddDataItem(L"Total Kernel Time:", CTimeSpan(ToSeconds(info.BasicInfo.TotalKernelTime.QuadPart)));
			AddDataItem(L"Total CPU Time:", CTimeSpan(ToSeconds(info.BasicInfo.TotalUserTime.QuadPart + info.BasicInfo.TotalKernelTime.QuadPart)));

			BeginWriteInfo(IDC_GROUP_IO);
			AddDataItem(L"Read Transfer:", CString(std::to_wstring(info.IoInfo.ReadTransferCount >> 10).c_str()) + L" KB");
			AddDataItem(L"Write Transfer:", CString(std::to_wstring(info.IoInfo.WriteTransferCount >> 10).c_str()) + L" KB");
			AddDataItem(L"Other Transfer:", CString(std::to_wstring(info.IoInfo.OtherTransferCount >> 10).c_str()) + L" KB");
			AddDataItem(L"Read Operations:", info.IoInfo.ReadOperationCount);
			AddDataItem(L"Write Operations:", info.IoInfo.WriteOperationCount);
			AddDataItem(L"Other Operations:", info.IoInfo.OtherOperationCount);
		}
	}
	{
		// update process list
		BYTE buffer[1 << 11];
		if (::QueryInformationJobObject(m_hJob.get(), JobObjectBasicProcessIdList, buffer, sizeof(buffer), nullptr)) {
			auto info = reinterpret_cast<JOBOBJECT_BASIC_PROCESS_ID_LIST*>(buffer);

			m_ProcessList.clear();
			m_ProcessList.reserve(info->NumberOfProcessIdsInList);
			for (DWORD i = 0; i < info->NumberOfProcessIdsInList; i++) {
				auto pid = static_cast<DWORD>(info->ProcessIdList[i]);
				ProcessInfo pi;
				pi.Name = GetProcessName(pid);
				pi.Id = pid;
				m_ProcessList.emplace_back(pi);
			}
			m_Processes.SetItemCountEx(static_cast<int>(info->NumberOfProcessIdsInList), LVSICF_NOSCROLL);
		}
	}
}

void CMainDlg::BeginWriteInfo(UINT id) {
	CRect rc;
	GetDlgItem(id).GetWindowRect(&rc);
	ScreenToClient(&rc);
	CClientDC dc(*this);
	dc.SelectFont(GetFont());
	CSize size;
	dc.GetTextExtent(L"A", 1, &size);
	rc.top += size.cy;
	rc.DeflateRect(2, 4);
	dc.FillSolidRect(&rc, ::GetSysColor(CTLCOLOR_DLG));

	rc.bottom = rc.top + size.cy;
	CRect rc2(rc);
	rc.right = rc.left + 120;
	rc2.left = rc.right + 10;
	m_InfoRectProperty = rc;
	m_InfoRectValue = rc2;
	m_Height = size.cy + 4;
}

void CMainDlg::RefreshSystemProcessList() {
	wil::unique_handle hSnapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
	if (!hSnapshot)
		return;

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);
	if (!::Process32First(hSnapshot.get(), &pe))
		return;

	m_ProcessCombo.ResetContent();

	// skip idle process (PID=0)
	while (::Process32Next(hSnapshot.get(), &pe)) {
		CString text;
		text.Format(L"%s (%d)", pe.szExeFile, pe.th32ProcessID);
		int n = m_ProcessCombo.AddString(text);
		m_ProcessCombo.SetItemData(n, pe.th32ProcessID);
	}
	m_ProcessCombo.SetCurSel(0);
}

void CMainDlg::UpdateUIForNewJob(BOOL reset) {
	UpdateJob();
	GetDlgItem(IDC_CREATEJOB).EnableWindow(reset);
	GetDlgItem(IDC_OPEN_JOB).EnableWindow(reset);
	CString handle;
	if(!reset)
		handle.Format(L"%u (0x%X)", HandleToUlong(m_hJob.get()), HandleToUlong(m_hJob.get()));
	SetDlgItemText(IDC_HANDLE, handle);
	GetDlgItem(IDC_JOBNAME).SendMessageW(EM_SETREADONLY, !reset);

	GetDlgItem(IDC_TERMINATE).EnableWindow(!reset);
	GetDlgItem(IDC_ADD_EXISTING_PROCESS).EnableWindow(!reset);
	GetDlgItem(IDC_CREATE_PROCESS).EnableWindow(!reset);
	GetDlgItem(IDC_BIND_IO_COMPLETION).EnableWindow(!reset);
	GetDlgItem(IDC_SET_LIMIT).EnableWindow(!reset);
	GetDlgItem(IDC_REMOVE_LIMIT).EnableWindow(!reset);
	GetDlgItem(IDC_CLOSE_HANDLE).EnableWindow(!reset);
	GetDlgItem(IDC_REFRESH_LIMITS).EnableWindow(!reset);

	if (reset)
		KillTimer(1);
	else
		SetTimer(1, 1500, nullptr);
}

void CMainDlg::UpdateJobLimits() {
	ATLASSERT(m_hJob);
	m_Limits.LockWindowUpdate();
	m_Limits.ResetContent();
	{
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;
		if (::QueryInformationJobObject(m_hJob.get(), JobObjectExtendedLimitInformation, &info, sizeof(info), nullptr)) {
			auto flags = info.BasicLimitInformation.LimitFlags;
			if (flags & JOB_OBJECT_LIMIT_ACTIVE_PROCESS)
				AddLimit(L"Active Processes: ", info.BasicLimitInformation.ActiveProcessLimit);
			if (flags & JOB_OBJECT_LIMIT_BREAKAWAY_OK)
				AddLimit(L"Breakaway OK");
			if (flags & JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION)
				AddLimit(L"Die on Unhandled Exception");
			if (flags & JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK)
				AddLimit(L"Silent Breakaway OK");
			if (flags & JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE)
				AddLimit(L"Kill on Job Close");
			if (flags & JOB_OBJECT_LIMIT_PRIORITY_CLASS)
				AddLimit(L"Priority Class: " + PriorityClassToString(info.BasicLimitInformation.PriorityClass));
			if (flags & JOB_OBJECT_LIMIT_SCHEDULING_CLASS)
				AddLimit(L"Scheduling Class: ", info.BasicLimitInformation.SchedulingClass);
			if (flags & JOB_OBJECT_LIMIT_AFFINITY)
				AddLimit(L"Affinity: ", info.BasicLimitInformation.Affinity);
			if (flags & JOB_OBJECT_LIMIT_JOB_MEMORY)
				AddLimit(L"Job Memory: " + std::to_wstring(info.JobMemoryLimit >> 20) + L" MB");
			if (flags & JOB_OBJECT_LIMIT_PROCESS_MEMORY)
				AddLimit(L"Process Memory: " + std::to_wstring(info.ProcessMemoryLimit >> 20) + L" MB");
			if (flags & JOB_OBJECT_LIMIT_WORKINGSET)
				AddLimit(L"Maximum Working Set: " + std::to_wstring(info.BasicLimitInformation.MaximumWorkingSetSize >> 20) + L" MB");
		}
	}
	{
		JOBOBJECT_BASIC_UI_RESTRICTIONS info;
		if (::QueryInformationJobObject(m_hJob.get(), JobObjectBasicUIRestrictions, &info, sizeof(info), nullptr)) {
			static struct {
				CString Name;
				DWORD Value;
			} uitypes[] = {
				{ L"Desktop", JOB_OBJECT_UILIMIT_DESKTOP },
				{ L"Read Clipboard", JOB_OBJECT_UILIMIT_READCLIPBOARD },
				{ L"Write Clipboard", JOB_OBJECT_UILIMIT_WRITECLIPBOARD },
				{ L"Global Atoms", JOB_OBJECT_UILIMIT_GLOBALATOMS },
				{ L"Exit Windows", JOB_OBJECT_UILIMIT_EXITWINDOWS },
				{ L"Display Settings", JOB_OBJECT_UILIMIT_DISPLAYSETTINGS },
				{ L"System Parameters", JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS },
				{ L"Handles", JOB_OBJECT_UILIMIT_HANDLES }
			};
			for (auto& ui : uitypes) {
				if (ui.Value & info.UIRestrictionsClass)
					AddLimit(L"User Interface - " + ui.Name);
			}
		}
	}
	m_Limits.LockWindowUpdate(FALSE);
}

void CMainDlg::AddLimit(PCWSTR name) {
	m_Limits.AddString(name);
}

void CMainDlg::AddLimit(const std::wstring& limit) {
	AddLimit(limit.c_str());
}

void CMainDlg::AddDataItem(PCWSTR property, const CTimeSpan& ts) {
	AddDataItem(property, ts.Format(L"%H:%M:%S"));
}

void CMainDlg::AddDataItem(PCWSTR property, const CString& value) {
	CClientDC dc(*this);
	dc.SetBkMode(TRANSPARENT);
	dc.SelectFont(GetFont());

	dc.DrawText(property, -1, m_InfoRectProperty, DT_RIGHT | DT_SINGLELINE);
	dc.DrawText(value, -1, m_InfoRectValue, DT_SINGLELINE | DT_LEFT);
	m_InfoRectProperty.OffsetRect(0, m_Height);
	m_InfoRectValue.OffsetRect(0, m_Height);
}

CString CMainDlg::GetProcessName(DWORD pid) {
	wil::unique_handle hProcess(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
	if (!hProcess)
		return L"<Unknown>";

	WCHAR path[MAX_PATH];
	DWORD size = _countof(path);
	if (!::QueryFullProcessImageName(hProcess.get(), 0, path, &size))
		return L"<Unknown>";

	return CString(::wcsrchr(path, L'\\') + 1);
}

CString CMainDlg::PriorityClassToString(int priority) {
	switch (priority) {
		case NORMAL_PRIORITY_CLASS:	return L"Normal";
		case ABOVE_NORMAL_PRIORITY_CLASS: return L"Above Normal";
		case BELOW_NORMAL_PRIORITY_CLASS: return L"Below Normal";
		case HIGH_PRIORITY_CLASS: return L"High";
		case IDLE_PRIORITY_CLASS: return L"Idle";
		case REALTIME_PRIORITY_CLASS: return L"Realtime";
	}
	return L"(Unknown)";
}

DWORD CMainDlg::DoMonitorJob() {
	for (;;) {
		DWORD message;
		ULONG_PTR key;
		LPOVERLAPPED data;
		if (::GetQueuedCompletionStatus(m_hCompletionPort.get(), &message, &key, &data, INFINITE)) {
			// handle notification
			switch (message) {
				case JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT:
					AddLog(L"Job Notification: Active process limit exceeded");
					break;

				case JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO:
					AddLog(L"Job Notification: Active processes is zero");
					break;

				case JOB_OBJECT_MSG_NEW_PROCESS:
					AddLog(L"Job Notification: New process created (PID: " + std::to_wstring(PtrToUlong(data)) + L")");
					break;

				case JOB_OBJECT_MSG_EXIT_PROCESS:
					AddLog(L"Job Notification: process exited (PID: " + std::to_wstring(PtrToUlong(data)) + L")");
					break;

				case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:
					AddLog(L"Job Notification: Process " + std::to_wstring(PtrToUlong(data)) + L" exited abnormally");
					break;

				case JOB_OBJECT_MSG_JOB_MEMORY_LIMIT:
					AddLog(L"Job Notification: Job memory limit exceed attempt by process " + std::to_wstring(PtrToUlong(data)));
					break;

				case JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT:
					AddLog(L"Job Notification: Process " + std::to_wstring(PtrToUlong(data)) + L" exceeded its memory limit");
					break;

				case JOB_OBJECT_MSG_END_OF_JOB_TIME:
					AddLog(L"Job time limit exceeded");
					break;

				case JOB_OBJECT_MSG_END_OF_PROCESS_TIME:
					AddLog(L"Process " + std::to_wstring(PtrToUlong(data)) + L" has exceeded its time limit");
					break;
			}
		}
	}

	return 0;
}

void CMainDlg::AddDataItem(PCWSTR property, const LARGE_INTEGER& li) {
	AddDataItem(property, CTimeSpan(li.QuadPart).Format(L"%H:%M:%S"));
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnCreateJob(WORD, WORD wID, HWND, BOOL&) {
	ATLASSERT(m_hJob == nullptr);

	CString name;
	GetDlgItemText(IDC_JOBNAME, name);

	m_hJob.reset(::CreateJobObject(nullptr, name));
	if (!m_hJob) {
		DisplayError(L"Failed to create job");
		return 0;
	}

	bool existing = ::GetLastError() == ERROR_ALREADY_EXISTS;
	CString createOrOpen = existing ? L"opened" : L"created";
	AddLog(L"Job " + createOrOpen + L" successfully");
	UpdateUIForNewJob();

	return 0;
}

LRESULT CMainDlg::OnOpenJob(WORD, WORD wID, HWND, BOOL&) {
	ATLASSERT(m_hJob == nullptr);

	CString name;
	GetDlgItemText(IDC_JOBNAME, name);
	if (name.IsEmpty()) {
		MessageBox(L"Opening an existing job requires a name", L"Job Monitor", MB_ICONWARNING);
		GetDlgItem(IDC_JOBNAME).SetFocus();
		return 0;
	}

	m_hJob.reset(::OpenJobObject(JOB_ALL_ACCESS, FALSE, name));
	if (!m_hJob) {
		m_hJob.reset(::OpenJobObject(JOB_OBJECT_QUERY, FALSE, name));
		if (!m_hJob) {
			DisplayError(L"Failed to open job");
			return 0;
		}
	}
	UpdateUIForNewJob();
	UpdateJob();

	return 0;
}

LRESULT CMainDlg::OnAddExistingProcess(WORD, WORD wID, HWND, BOOL&) {
	DWORD pid = static_cast<DWORD>(m_ProcessCombo.GetItemData(m_ProcessCombo.GetCurSel()));
	wil::unique_handle hProcess(::OpenProcess(PROCESS_SET_QUOTA | PROCESS_TERMINATE, FALSE, pid));
	if (!hProcess) {
		DisplayError(L"Failed to open process");
		return 0;
	}

	if (!::AssignProcessToJobObject(m_hJob.get(), hProcess.get())) {
		DisplayError(L"Failed to assign process to job");
		return 0;
	}

	AddLog(L"Added process " + std::to_wstring(pid) + L" successfully");

	return 0;
}

LRESULT CMainDlg::OnTerminateJob(WORD, WORD wID, HWND, BOOL&) {
	ATLASSERT(m_hJob);

	if (!::TerminateJobObject(m_hJob.get(), 1)) {
		DisplayError(L"Failed to terminate job");
		return 0;
	}

	AddLog(L"Job terminated successfully");
	return 0;
}

LRESULT CMainDlg::OnRefresh(WORD, WORD wID, HWND, BOOL&) {
	RefreshSystemProcessList();

	return 0;
}

LRESULT CMainDlg::OnBrowse(WORD, WORD wID, HWND, BOOL&) {
	CSimpleFileDialog dlg(TRUE, L".exe", nullptr, OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
		L"Executables (*.exe)\0*.exe\0AllFiles (*.*)\0*.*\0", *this);
	if (dlg.DoModal() == IDOK) {
		SetDlgItemText(IDC_EXEPATH, CString(L"\"") + dlg.m_szFileName + L"\"");
	}
	return 0;
}

LRESULT CMainDlg::OnSetLimit(WORD, WORD wID, HWND, BOOL&) {
	BOOL success = FALSE;
	CString text;
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;
	::QueryInformationJobObject(m_hJob.get(), JobObjectExtendedLimitInformation, &info, sizeof(info), nullptr);
	bool isExtendedLimit = true;
	auto type = static_cast<JobLimitType>(m_LimitsCombo.GetItemData(m_LimitsCombo.GetCurSel()));
	switch (type) {
		case JobLimitType::BreakawayOk:
			info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_BREAKAWAY_OK;
			break;

		case JobLimitType::SilentBreakawayOk:
			info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
			break;

		case JobLimitType::DieOnUnhandledException:
			info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
			break;

		case JobLimitType::KillOnJobClose:
			info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
			break;

		case JobLimitType::ActiveProcesses:
			m_EditValue.GetWindowText(text);
			info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
			info.BasicLimitInformation.ActiveProcessLimit = _wtoi(text);
			break;

		case JobLimitType::Affinity:
			m_EditValue.GetWindowText(text);
			info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_AFFINITY;
			info.BasicLimitInformation.Affinity = ParseNumber<ULONG_PTR>(text);
			break;

		case JobLimitType::JobMemory:
			m_EditValue.GetWindowText(text);
			info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_JOB_MEMORY;
			info.JobMemoryLimit = ParseNumber<SIZE_T>(text) << m_UnitsCombo.GetItemData(m_UnitsCombo.GetCurSel());
			break;

		case JobLimitType::MaxWorkingSet:
			m_EditValue.GetWindowText(text);
			info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_WORKINGSET;
			info.BasicLimitInformation.MinimumWorkingSetSize = 20 << 12;	// 20 pages minimum
			info.BasicLimitInformation.MaximumWorkingSetSize = ParseNumber<SIZE_T>(text) << m_UnitsCombo.GetItemData(m_UnitsCombo.GetCurSel());
			break;

		case JobLimitType::ProcessMemory:
			m_EditValue.GetWindowText(text);
			info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
			info.ProcessMemoryLimit = ParseNumber<SIZE_T>(text) << m_UnitsCombo.GetItemData(m_UnitsCombo.GetCurSel());
			break;

		case JobLimitType::PriorityClass:
			info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PRIORITY_CLASS;
			info.BasicLimitInformation.PriorityClass = (DWORD)m_ValueCombo.GetItemData(m_ValueCombo.GetCurSel());
			break;

		case JobLimitType::UserInterface_All:
		case JobLimitType::UserInterface_Desktop:
		case JobLimitType::UserInterface_ReadClipboard:
		case JobLimitType::UserInterface_WriteClipboard:
		case JobLimitType::UserInterface_Handles:
		case JobLimitType::UserInterface_SystemParameters:
		case JobLimitType::UserInterface_DisplaySettings:
		case JobLimitType::UserInterface_GlobalAtoms:
		{
			isExtendedLimit = false;
			JOBOBJECT_BASIC_UI_RESTRICTIONS info;
			::QueryInformationJobObject(m_hJob.get(), JobObjectBasicUIRestrictions, &info, sizeof(info), nullptr);
			info.UIRestrictionsClass |= ((DWORD)type - 0x100);
			success = ::SetInformationJobObject(m_hJob.get(), JobObjectBasicUIRestrictions, &info, sizeof(info));
			break;
		}

		default:
			ATLASSERT(false);
			return 0;
	}

	m_LimitsCombo.GetLBText(m_LimitsCombo.GetCurSel(), text);

	if(isExtendedLimit)
		success = ::SetInformationJobObject(m_hJob.get(), JobObjectExtendedLimitInformation, &info, sizeof(info));

	if (!success) {
		DisplayError(L"Failed to set limit " + text);
	}
	else {
		AddLog(L"Set limit " + text + L" successfully");
		UpdateJobLimits();
	}
	return 0;
}

LRESULT CMainDlg::OnRemoveLimit(WORD, WORD wID, HWND, BOOL&) {
	ATLASSERT(m_hJob);

	auto type = static_cast<JobLimitType>(m_LimitsCombo.GetItemData(m_LimitsCombo.GetCurSel()));
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;
	::QueryInformationJobObject(m_hJob.get(), JobObjectExtendedLimitInformation, &info, sizeof(info), nullptr);
	BOOL success = FALSE;
	bool isLimit = true;

	switch (type) {
		case JobLimitType::ActiveProcesses:
			info.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
			break;

		case JobLimitType::PriorityClass:
			info.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_PRIORITY_CLASS;
			break;

		case JobLimitType::BreakawayOk:
			info.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_BREAKAWAY_OK;
			break;

		case JobLimitType::SilentBreakawayOk:
			info.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
			break;

		case JobLimitType::KillOnJobClose:
			info.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
			break;

		case JobLimitType::UserInterface_All:
		case JobLimitType::UserInterface_Desktop:
		case JobLimitType::UserInterface_ReadClipboard:
		case JobLimitType::UserInterface_WriteClipboard:
		case JobLimitType::UserInterface_Handles:
		case JobLimitType::UserInterface_SystemParameters:
		case JobLimitType::UserInterface_DisplaySettings:
		case JobLimitType::UserInterface_GlobalAtoms:
		{
			isLimit = false;
			JOBOBJECT_BASIC_UI_RESTRICTIONS info;
			::QueryInformationJobObject(m_hJob.get(), JobObjectBasicUIRestrictions, &info, sizeof(info), nullptr);
			info.UIRestrictionsClass &= ~((DWORD)type - 0x100);
			success = ::SetInformationJobObject(m_hJob.get(), JobObjectBasicUIRestrictions, &info, sizeof(info));
			break;
		}

		default:
			ATLASSERT(false);
			return 0;
	}

	CString text;
	m_LimitsCombo.GetLBText(m_LimitsCombo.GetCurSel(), text);

	if (isLimit) {
		success = ::SetInformationJobObject(m_hJob.get(), JobObjectExtendedLimitInformation, &info, sizeof(info));
	}

	if (success) {
		AddLog(L"Removed " + text + L" successfully");
		UpdateJobLimits();
	}
	else {
		DisplayError(L"Failed to remove limit " + text);
	}

	return 0;
}

LRESULT CMainDlg::OnRefreshLimits(WORD, WORD wID, HWND, BOOL&) {
	UpdateJobLimits();
	return 0;
}

LRESULT CMainDlg::OnBindToIoCompletion(WORD, WORD wID, HWND, BOOL&) {
	ATLASSERT(m_hJob);
	ATLASSERT(m_hCompletionPort == nullptr);

	wil::unique_handle hCompletionPort(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0));
	if (!hCompletionPort) {
		DisplayError(L"Failed to create I/O completion port");
		return 0;
	}

	JOBOBJECT_ASSOCIATE_COMPLETION_PORT info;
	info.CompletionKey = 0;
	info.CompletionPort = hCompletionPort.get();
	if (!::SetInformationJobObject(m_hJob.get(), JobObjectAssociateCompletionPortInformation, &info, sizeof(info))) {
		DisplayError(L"Failed to associate completion port");
		return 0;
	}

	m_hCompletionPort = std::move(hCompletionPort);

	// create a thread to monitor notifications
	wil::unique_handle hThread(::CreateThread(nullptr, 0, [](auto p) {
		return static_cast<CMainDlg*>(p)->DoMonitorJob();
	}, this, 0, nullptr));
	ATLASSERT(hThread);

	AddLog(L"Completion port associated successfully");
	GetDlgItem(IDC_BIND_IO_COMPLETION).EnableWindow(FALSE);

	return 0;
}

LRESULT CMainDlg::OnCloseHandle(WORD, WORD wID, HWND, BOOL&) {
	m_hJob.reset();
	m_hCompletionPort.reset();
	m_Log.DeleteAllItems();
	UpdateUIForNewJob(true);

	return 0;
}

LRESULT CMainDlg::OnCreateProcess(WORD, WORD, HWND, BOOL&) {
	ATLASSERT(m_hJob);

	CString path;
	GetDlgItemText(IDC_EXEPATH, path);
	if (path.IsEmpty()) {
		MessageBox(L"Please specify executable path", L"Job Monitor", MB_ICONWARNING);
		return 0;
	}

	PROCESS_INFORMATION pi;
	STARTUPINFO si = { sizeof(si) };
	if (!::CreateProcess(nullptr, path.GetBuffer(), nullptr, nullptr, FALSE, CREATE_BREAKAWAY_FROM_JOB, nullptr, nullptr, &si, &pi) &&
		// if breakaway creation fails, it means some parent job prevents it. Try without a breakaway
		!::CreateProcess(nullptr, path.GetBuffer(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
		DisplayError(L"Failed to create process");
		return 0;
	}

	if (!::AssignProcessToJobObject(m_hJob.get(), pi.hProcess)) {
		DisplayError(L"Failed to assign process to job");
	}
	else {
		AddLog(L"Added process " + std::to_wstring(pi.dwProcessId) + L" successfully");
	}
	::CloseHandle(pi.hProcess);
	::CloseHandle(pi.hThread);

	return 0;
}
