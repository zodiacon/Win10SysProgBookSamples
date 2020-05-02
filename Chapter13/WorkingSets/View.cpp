// View.cpp : implementation of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "View.h"
#include <algorithm>
#include "SortHelper.h"

CView::CView(CUpdateUIBase* ui) : m_UI(ui) {
}

BOOL CView::PreTranslateMessage(MSG* pMsg) {
	return FALSE;
}

void CView::Refresh() {
	wil::unique_handle hSnapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
	if (!hSnapshot)
		return;

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);
	::Process32First(hSnapshot.get(), &pe);

	m_Items.clear();
	m_Items.reserve(512);

	while (::Process32Next(hSnapshot.get(), &pe)) {
		// attempt to open a handle to the process
		wil::unique_handle hProcess(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID));
		if (!hProcess && m_ShowOnlyAccessibleProcesses)
			continue;

		ProcessInfo pi;
		pi.Id = pe.th32ProcessID;
		pi.ImageName = pe.szExeFile;

		if (hProcess) {
			::GetProcessMemoryInfo(hProcess.get(), (PROCESS_MEMORY_COUNTERS*)&pi.Counters, sizeof(pi.Counters));
			::GetProcessWorkingSetSizeEx(hProcess.get(), &pi.MinWorkingSet, &pi.MaxWorkingSet, &pi.WorkingSetFlags);
			pi.CountersAvailable = true;
		}
		m_Items.push_back(pi);
	}

	DoSort(GetSortInfo(*this));
	SetItemCountEx(static_cast<int>(m_Items.size()), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
	RedrawItems(GetTopIndex(), GetTopIndex() + GetCountPerPage());
}

void CView::DoSort(const SortInfo* si) {
	if (si == nullptr)
		return;

	std::sort(m_Items.begin(), m_Items.end(), [si](auto& item1, auto& item2) {
		return CompareItems(item1, item2, si->SortColumn, si->SortAscending);
		});
}

CString CView::GetColumnText(HWND hWnd, int row, int column) const {
	auto& item = m_Items[row];
	CString result;

	switch (column) {
		case 0: return item.ImageName;
		case 1: result.Format(L"%u (0x%X)", item.Id, item.Id); break;
		case 2: return item.CountersAvailable ? (FormatSize(item.Counters.WorkingSetSize >> 10) + L" KB") : L"";
		case 3: return item.CountersAvailable ? (FormatSize(item.Counters.PeakWorkingSetSize >> 10) + L" KB") : L"";
		case 4: return item.CountersAvailable ? (FormatSize(item.MinWorkingSet >> 10) + L" KB") : L"";
		case 5: return item.CountersAvailable ? (FormatSize(item.MaxWorkingSet >> 10) + L" KB") : L"";
		case 6: return item.CountersAvailable ? WSFlagsToString(item.WorkingSetFlags) : L"";
		case 7: return item.CountersAvailable ? (FormatSize(item.Counters.PrivateUsage >> 10) + L" KB") : L"";
		case 8: return item.CountersAvailable ? (FormatSize(item.Counters.PeakPagefileUsage >> 10) + L" KB") : L"";
		case 9: return item.CountersAvailable ? (FormatSize(item.Counters.PageFaultCount)) : L"";
	}

	return result;
}

CString CView::FormatSize(SIZE_T size) {
	CString result;
	result.Format(L"%zu", size);
	int i = 3;
	while (result.GetLength() - i > 0) {
		result = result.Left(result.GetLength() - i) + L"," + result.Right(i);
		i += 4;
	}
	return result;
}

CString CView::WSFlagsToString(DWORD flags) {
	CString result;
	static const struct {
		DWORD flag;
		PCWSTR text;
	} data[] = {
		{ QUOTA_LIMITS_HARDWS_MIN_DISABLE, L"Min Disable" },
		{ QUOTA_LIMITS_HARDWS_MIN_ENABLE, L"Min Enable" },
		{ QUOTA_LIMITS_HARDWS_MAX_DISABLE, L"Max Disable" },
		{ QUOTA_LIMITS_HARDWS_MAX_ENABLE, L"Max Enable" },
	};

	for (const auto& item : data) {
		if (item.flag & flags)
			result += item.text + CString(L", ");
	}
	return result.IsEmpty() ? L"" : result.Left(result.GetLength() - 2);
}

bool CView::CompareItems(const ProcessInfo& p1, const ProcessInfo& p2, int col, bool asc) {
	switch (col) {
		case 0: return SortHelper::SortStrings(p1.ImageName, p2.ImageName, asc);
		case 1: return SortHelper::SortNumbers(p1.Id, p2.Id, asc);
		case 2: 
			if(p1.CountersAvailable && p2.CountersAvailable)
				return SortHelper::SortNumbers(p1.Counters.WorkingSetSize, p2.Counters.WorkingSetSize, asc);
			break;
		case 3:
			if (p1.CountersAvailable && p2.CountersAvailable)
				return SortHelper::SortNumbers(p1.Counters.PeakWorkingSetSize, p2.Counters.PeakWorkingSetSize, asc);
			break;
		case 4:
			if (p1.CountersAvailable && p2.CountersAvailable)
				return SortHelper::SortNumbers(p1.MinWorkingSet, p2.MinWorkingSet, asc);
			break;
		case 5:
			if (p1.CountersAvailable && p2.CountersAvailable)
				return SortHelper::SortNumbers(p1.MaxWorkingSet, p2.MaxWorkingSet, asc);
			break;
		case 6:
			if (p1.CountersAvailable && p2.CountersAvailable)
				return SortHelper::SortNumbers(p1.WorkingSetFlags, p2.WorkingSetFlags, asc);
			break;
		case 7:
			if (p1.CountersAvailable && p2.CountersAvailable)
				return SortHelper::SortNumbers(p1.Counters.PrivateUsage, p2.Counters.PrivateUsage, asc);
			break;
		case 8:
			if (p1.CountersAvailable && p2.CountersAvailable)
				return SortHelper::SortNumbers(p1.Counters.PeakPagefileUsage, p2.Counters.PeakPagefileUsage, asc);
			break;
		case 9:
			if (p1.CountersAvailable && p2.CountersAvailable)
				return SortHelper::SortNumbers(p1.Counters.PageFaultCount, p2.Counters.PageFaultCount, asc);
			break;
		default:
			ATLASSERT(false);
			return false;
	}
	return p1.CountersAvailable;
}

LRESULT CView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	DefWindowProc();
	
	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP | LVS_EX_INFOTIP);

	InsertColumn(0, L"Name", LVCFMT_LEFT, 200);
	InsertColumn(1, L"PID", LVCFMT_RIGHT, 100);
	InsertColumn(2, L"Working Set", LVCFMT_RIGHT, 100);
	InsertColumn(3, L"Peak WS", LVCFMT_RIGHT, 100);
	InsertColumn(4, L"Min WS", LVCFMT_RIGHT, 100);
	InsertColumn(5, L"Max WS", LVCFMT_RIGHT, 100);
	InsertColumn(6, L"WS Flags", LVCFMT_LEFT, 150);
	InsertColumn(7, L"Commit Charge", LVCFMT_RIGHT, 100);
	InsertColumn(8, L"Peak Commit", LVCFMT_RIGHT, 100);
	InsertColumn(9, L"Page Faults", LVCFMT_RIGHT, 100);

	Refresh();

	SetTimer(1, 1000, nullptr);

	return 0;
}

LRESULT CView::OnTimer(UINT, WPARAM id, LPARAM, BOOL&) {
	if (id == 1)
		Refresh();

	return 0;
}

LRESULT CView::OnViewRefresh(WORD, WORD, HWND, BOOL&) {
	Refresh();

	return 0;
}

LRESULT CView::OnShowAccessibleProcesses(WORD, WORD, HWND, BOOL&) {
	m_ShowOnlyAccessibleProcesses = !m_ShowOnlyAccessibleProcesses;
	m_UI->UISetCheck(ID_VIEW_SHOWONLYACCESSIBLEPROCESSES, m_ShowOnlyAccessibleProcesses);
	Refresh();

	return 0;
}

LRESULT CView::OnEmptyWorkingSet(WORD, WORD, HWND, BOOL&) {
	auto index = GetSelectedIndex();
	ATLASSERT(index >= 0);

	const auto& item = m_Items[index];
	wil::unique_handle hProcess(::OpenProcess(PROCESS_SET_QUOTA, FALSE, item.Id));
	if (!hProcess) {
		AtlMessageBox(*this, L"Failed to open process", IDR_MAINFRAME, MB_ICONERROR);
		return 0;
	}

	if (!::SetProcessWorkingSetSize(hProcess.get(), (SIZE_T)-1, (SIZE_T)-1)) {
		AtlMessageBox(*this, L"Failed to empty working set", IDR_MAINFRAME, MB_ICONERROR);
	}
	return 0;
}

LRESULT CView::OnItemChanged(int, LPNMHDR, BOOL&) {
	auto index = GetSelectedIndex();
	m_UI->UIEnable(ID_PROCESS_EMPTYWORKINGSET, index >= 0 && m_Items[index].CountersAvailable);

	return 0;
}

