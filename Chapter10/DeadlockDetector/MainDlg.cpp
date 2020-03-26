// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "MainDlg.h"
#include <algorithm>

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	m_Tree.Attach(GetDlgItem(IDC_CHAINS));
	m_ProcCombo.Attach(GetDlgItem(IDC_PROCESSES));
	m_Images.Create(16, 16, ILC_COLOR32, 32, 16);
	m_ProcCombo.SetImageList(m_Images);

	((CButton)GetDlgItem(IDC_REFRESH)).SetIcon(AtlLoadIconImage(IDI_REFRESH, 0, 16, 16));

	auto comLib = ::GetModuleHandle(L"ole32");
	if (comLib) {
		::RegisterWaitChainCOMCallback(
			(PCOGETCALLSTATE)::GetProcAddress(comLib, "CoGetCallState"),
			(PCOGETACTIVATIONSTATE)::GetProcAddress(comLib, "CoGetActivationState"));
	}

	InitProcessesCombo();

	return TRUE;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnDetect(WORD, WORD wID, HWND, BOOL&) {
	auto hWct = ::OpenThreadWaitChainSession(0, nullptr);
	if (hWct == nullptr) {
		AtlMessageBox(*this, L"Failed to open WCT session", IDR_MAINFRAME, MB_ICONERROR);
		return 0;
	}

	auto pid = (DWORD)m_ProcCombo.GetItemData(m_ProcCombo.GetCurSel());
	auto threads = EnumThreads(pid);

	m_Tree.DeleteAllItems();

	int failures = 0;
	for (auto& tid : threads) {
		if (!DoWaitChain(hWct, tid))
			failures++;
	}
	if (failures == threads.size()) {
		AtlMessageBox(*this, L"Failed to analyze wait chain. (try running elevated)", 
			IDR_MAINFRAME, MB_ICONEXCLAMATION);
	
	}
	::CloseThreadWaitChainSession(hWct);
	return 0;
}

LRESULT CMainDlg::OnRefresh(WORD, WORD wID, HWND, BOOL&) {
	InitProcessesCombo();
	return 0;
}

void CMainDlg::InitProcessesCombo() {
	auto hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
		return;

	m_ProcCombo.ResetContent();
	m_Images.RemoveAll();
	m_Images.AddIcon(AtlLoadSysIcon(IDI_APPLICATION));

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);

	::Process32First(hSnapshot, &pe);
	struct Item {
		CString Name;
		DWORD Pid;
	};

	std::vector<Item> names;
	WCHAR path[MAX_PATH];
	while (::Process32Next(hSnapshot, &pe)) {
		Item item;
		item.Name = pe.szExeFile;
		item.Pid = pe.th32ProcessID;
		names.push_back(item);
	}

	std::sort(names.begin(), names.end(), [](auto& p1, auto& p2) {
		return p1.Name.CompareNoCase(p2.Name) < 0;
		});

	for(auto& p : names) {
		auto hProcess = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, p.Pid);
		int image = 0;
		if (hProcess) {
			DWORD size = MAX_PATH;
			if (::QueryFullProcessImageName(hProcess, 0, path, &size)) {
				HICON hIcon = nullptr;
				::ExtractIconEx(path, 0, nullptr, &hIcon, 1);
				if (hIcon)
					image = m_Images.AddIcon(hIcon);
			}
			::CloseHandle(hProcess);
		}
		m_ProcCombo.AddItem(p.Name, image, image, 0, p.Pid);
	}
	::CloseHandle(hSnapshot);

	m_ProcCombo.SetCurSel(0);
}

std::vector<DWORD> CMainDlg::EnumThreads(DWORD pid) {
	std::vector<DWORD> threads;

	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
		return threads;

	threads.reserve(16);

	THREADENTRY32 te;
	te.dwSize = sizeof(te);

	::Thread32First(hSnapshot, &te);

	do {
		if (te.th32OwnerProcessID == pid)
			threads.push_back(te.th32ThreadID);
	} while (::Thread32Next(hSnapshot, &te));

	::CloseHandle(hSnapshot);

	return threads;
}

bool CMainDlg::DoWaitChain(HWCT hWct, DWORD tid) {
	WAITCHAIN_NODE_INFO nodes[WCT_MAX_NODE_COUNT];
	DWORD nodeCount = WCT_MAX_NODE_COUNT;
	BOOL cycle;
	auto success = ::GetThreadWaitChain(hWct, 0, WCTP_GETINFO_ALL_FLAGS, tid, &nodeCount, nodes, &cycle);
	if(success) {
		ParseThreadNodes(nodes, nodeCount, cycle);
	}
	return success;
}

void CMainDlg::ParseThreadNodes(const WAITCHAIN_NODE_INFO* nodes, DWORD count, bool cycle) {
	static PCWSTR objectTypes[] = {
		L"Critical Section",
		L"Send Message",
		L"Mutex",
		L"ALPC",
		L"COM",
		L"Thread Wait",
		L"Process Wait",
		L"Thread",
		L"COM Activation",
		L"Unknown",
		L"Socket",
		L"SMB",
	};

	static PCWSTR statusTypes[] = {
		L"No Access",
		L"Running",
		L"Blocked",
		L"PID only",
		L"PID only RPCSS",
		L"Owned",
		L"Not Owned",
		L"Abandoned",
		L"Unknown",
		L"Error"
	};

	HTREEITEM hCurrentNode = TVI_ROOT;
	CString text;

	for (DWORD i = 0; i < count; i++) {
		auto& node = nodes[i];
		auto type = node.ObjectType;
		auto status = node.ObjectStatus;
	
		switch (type) {
			case WctThreadType:
				text.Format(L"Thread %u (PID: %u) Wait: %u (%s)",
					node.ThreadObject.ThreadId, node.ThreadObject.ProcessId,
					node.ThreadObject.WaitTime, statusTypes[status - 1]);
				break;

			case WctCriticalSectionType:
			case WctMutexType:
			case WctThreadWaitType:
			case WctProcessWaitType:
				// waitable objects
				text.Format(L"%s (%s) Name: %s", 
					objectTypes[type - 1], statusTypes[status - 1], node.LockObject.ObjectName);
				break;

			default:
				// other objects
				text.Format(L"%s (%s)", objectTypes[type - 1], statusTypes[node.ObjectStatus - 1]);
				break;
		}
		auto hOld = hCurrentNode;
		hCurrentNode = m_Tree.InsertItem(text, hCurrentNode, TVI_LAST);
		m_Tree.Expand(hOld, TVE_EXPAND);
	}
	if (cycle) {
		m_Tree.InsertItem(L"Deadlock!", hCurrentNode, TVI_LAST);
		m_Tree.Expand(hCurrentNode, TVE_EXPAND);
	}
}
