// View.cpp : implementation of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "View.h"
#include <atltime.h>

BOOL CView::PreTranslateMessage(MSG* pMsg) {
	pMsg;
	return FALSE;
}

void CView::Refresh() {
	m_ProcMgr.Refresh();
	m_List.SetItemCountEx(static_cast<int>(m_ProcMgr.GetProcesses().size()), LVSICF_NOINVALIDATEALL| LVSICF_NOSCROLL);
	m_List.RedrawItems(m_List.GetTopIndex(), m_List.GetCountPerPage() + m_List.GetTopIndex());
}

LRESULT CView::OnTimer(UINT, WPARAM id, LPARAM, BOOL&) {
	if (id == 1)
		Refresh();

	return 0;
}

LRESULT CView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	m_List.Create(*this, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		LVS_REPORT | LVS_SINGLESEL | LVS_OWNERDATA, 0, 123);
	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, 0);

	struct {
		PCWSTR Header;
		int Width;
		int Format = LVCFMT_LEFT;
	} columns[] = {
		{ L"Name", 200 },
		{ L"PID", 100, LVCFMT_RIGHT },
		{ L"Session", 80, LVCFMT_RIGHT },
		{ L"User Name", 150 },
		{ L"Threads", 80, LVCFMT_RIGHT },
		{ L"Handles", 80, LVCFMT_RIGHT },
		{ L"Working Set", 100, LVCFMT_RIGHT },
		{ L"CPU Time", 120, LVCFMT_RIGHT },
	};

	int i = 0;
	for (auto& col : columns)
		m_List.InsertColumn(i++, col.Header, col.Format, col.Width);

	Refresh();
	SetTimer(1, 1000, nullptr);

	return 0;
}

LRESULT CView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	int cx = GET_X_LPARAM(lParam), cy = GET_Y_LPARAM(lParam);
	if (m_List)
		m_List.MoveWindow(0, 0, cx, cy);
	return 0;
}

LRESULT CView::OnGetDispInfo(int, LPNMHDR pnmh, BOOL&) {
	auto lv = (NMLVDISPINFO*)pnmh;
	auto& item = lv->item;

	if (lv->item.mask & LVIF_TEXT) {
		const auto& data = m_ProcMgr.GetProcesses()[item.iItem];

		switch (item.iSubItem) {
			case 0:	// name
				item.pszText = (PWSTR)(PCWSTR)data->Name;
				break;

			case 1:	// pid
				StringCchPrintf(item.pszText, item.cchTextMax, L"%d", data->Id);
				break;

			case 2:	// session
				StringCchPrintf(item.pszText, item.cchTextMax, L"%d", data->SessionId);
				break;

			case 3:	// user name
				item.pszText = (PWSTR)(PCWSTR)data->UserName;
				break;

			case 4:	// threads
				StringCchPrintf(item.pszText, item.cchTextMax, L"%d", data->ThreadCount);
				break;

			case 5:	// handles
				StringCchPrintf(item.pszText, item.cchTextMax, L"%d", data->HandleCount);
				break;

			case 6:	// working set
				StringCchPrintf(item.pszText, item.cchTextMax, L"%d KB", data->WorkingSet >> 10);
				break;

			case 7:	// CPU Time
				StringCchPrintf(item.pszText, item.cchTextMax, L"%ws", 
					(PCWSTR)CTimeSpan((data->KernelTime + data->UserTime) / 10000000).Format(L"%D:%H:%M:%S"));
				break;
		}
	}

	return 0;
}
