// View.cpp : implementation of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "resource.h"

#include "View.h"
#include <appmodel.h>
#include <Shobjidl.h>

using namespace winrt;
using namespace winrt::Windows::Management::Deployment;
using namespace winrt::Windows::ApplicationModel;

BOOL CView::PreTranslateMessage(MSG* pMsg) {
	if (m_SearchBar.PreTranslateMessage(pMsg))
		return TRUE;

	return FALSE;
}

bool CView::CanExecuteApp() const {
	auto selected = m_List.GetSelectedIndex();
	return selected >= 0 && !GetItem(selected).IsFramework;
}

bool CView::CanCopy() const {
	return m_List.GetSelectedIndex() >= 0;
}

const AppItem & CView::GetItem(int index) const {
	return *m_VisiblePackages[index].get();
}

CString CView::VersionToString(const PackageVersion& version) {
	CString text;
	text.Format(L"%d.%d.%d.%d", (int)version.Major, (int)version.Minor, (int)version.Build, (int)version.Revision);
	return text;
}

bool CView::RunApp(PCWSTR fullPackageName) {
	PACKAGE_INFO_REFERENCE pir;
	int error = ::OpenPackageInfoByFullName(fullPackageName, 0, &pir);
	if (error != ERROR_SUCCESS)
		return false;

	bool success = false;
	do {
		UINT32 len = 0;
		error = ::GetPackageApplicationIds(pir, &len, nullptr, nullptr);
		if (error != ERROR_INSUFFICIENT_BUFFER)
			break;

		auto buffer = std::make_unique<BYTE[]>(len);
		UINT32 count;
		error = ::GetPackageApplicationIds(pir, &len, buffer.get(), &count);
		if (error != ERROR_SUCCESS)
			break;

		CComPtr<IApplicationActivationManager> mgr;
		auto hr = mgr.CoCreateInstance(CLSID_ApplicationActivationManager);
		if (FAILED(hr))
			break;

		DWORD pid;
		hr = mgr->ActivateApplication((PCWSTR)(buffer.get() + sizeof(ULONG_PTR)), nullptr, AO_NOERRORUI, &pid);
		success = SUCCEEDED(hr);
	} while (false);

	::ClosePackageInfo(pir);

	return success;
}

void CView::DoSort() {
	std::sort(m_VisiblePackages.begin(), m_VisiblePackages.end(),
		[this](auto& i1, auto& i2) { return CompareItems(*i1.get(), *i2.get()); });
}

bool CView::CompareItems(const AppItem & p1, const AppItem & p2) const {
	switch (m_SortColumn) {
		case 0:		// name
			return CompareStrings(p1.Name, p2.Name);

		case 1:		// framework?
			return m_SortAscending ? p1.IsFramework < p2.IsFramework : p1.IsFramework > p2.IsFramework;

		case 2:		// publisher
			return CompareStrings(p1.Publisher, p2.Publisher);

		case 3:
			return CompareVersions(p1.Version, p2.Version);

		case 4:
			return m_SortAscending ? p1.InstalledDate < p2.InstalledDate : p1.InstalledDate > p2.InstalledDate;

		case 5:		// install location
			return CompareStrings(p1.InstalledLocation, p2.InstalledLocation);
	}
	return false;
}

bool CView::CompareStrings(const wchar_t * s1, const wchar_t * s2) const {
	return m_SortAscending ? ::_wcsicmp(s2, s1) > 0 : ::_wcsicmp(s2, s1) < 0;
}

bool CView::CompareVersions(const PackageVersion & v1, const PackageVersion & v2) const {
	uint64_t ver1 = ((uint64_t)v1.Major << 48) | ((uint64_t)v1.Minor << 32);
	uint64_t ver2 = ((uint64_t)v2.Major << 48) | ((uint64_t)v2.Minor << 32);

	return m_SortAscending ? ver2 > ver1 : ver1 > ver2;
}

LRESULT CView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	m_SearchBar.Create(m_hWnd);
	m_SearchBar.ShowWindow(SW_SHOW);
	m_List.Create(m_hWnd, rcDefault, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPCHILDREN | LVS_REPORT | LVS_OWNERDATA | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
		0, 123);

	m_List.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT);

	m_List.InsertColumn(0, L"Package Name", 0, 250);
	m_List.InsertColumn(1, L"Framework?", 0, 80);
	m_List.InsertColumn(2, L"Publisher", 0, 400);
	m_List.InsertColumn(3, L"Version", 0, 130);
	m_List.InsertColumn(4, L"Install Date", 0, 180);
	m_List.InsertColumn(5, L"Install Location", 0, 550);

	auto packages = PackageManager().FindPackagesForUser(L"");
	for (auto package : packages) {
		auto item = std::make_shared<AppItem>();
		item->InstalledLocation = package.InstalledLocation().Path().c_str();
		item->FullName = package.Id().FullName().c_str();
		item->InstalledDate = package.InstalledDate();
		item->IsFramework = package.IsFramework();
		item->Name = package.Id().Name().c_str();
		item->Publisher = package.Id().Publisher().c_str();
		item->Version = package.Id().Version();

		m_AllPackages.push_back(item);
	}

	m_VisiblePackages = m_AllPackages;
	m_List.SetItemCount(static_cast<int>(m_VisiblePackages.size()));

	m_pUpdateUI = (CUpdateUIBase*)(((LPCREATESTRUCT)lParam)->lpCreateParams);

	return 0;
}

LRESULT CView::OnSize(UINT, WPARAM, LPARAM lParam, BOOL &) {
	if (m_SearchBar.IsWindow()) {
		int cx = GET_X_LPARAM(lParam), cy = GET_Y_LPARAM(lParam);
		RECT rc;
		m_SearchBar.GetClientRect(&rc);
		m_SearchBar.MoveWindow(0, 0, rc.right, rc.bottom);
		m_List.MoveWindow(0, rc.bottom, cx, cy - rc.bottom);
	}
	return 0;
}

LRESULT CView::OnGetDispInfo(int, LPNMHDR hdr, BOOL &) {
	auto lv = reinterpret_cast<NMLVDISPINFO*>(hdr);
	auto& item = lv->item;
	auto index = item.iItem;
	auto col = item.iSubItem;
	auto& data = GetItem(index);

	if (lv->item.mask & LVIF_TEXT) {
		switch (col) {
			case 0:	// package name
				::StringCchCopy(item.pszText, item.cchTextMax, data.Name);
				break;

			case 1:	// framework?
				if (data.IsFramework)
					item.pszText = L"Yes";
				break;

			case 2:	// publisher
				::StringCchCopy(item.pszText, item.cchTextMax, data.Publisher);
				break;

			case 3:	// version
				::StringCchCopy(item.pszText, item.cchTextMax, VersionToString(data.Version));
				break;

			case 4:	// install date
				::StringCchCopy(item.pszText, item.cchTextMax, CTime(clock::to_FILETIME(data.InstalledDate)).Format(L"%c"));
				break;

			case 5:	// install location
				::StringCchCopy(item.pszText, item.cchTextMax, data.InstalledLocation);
				break;
		}
	}

	return 0;
}

LRESULT CView::OnFind(WORD, WORD, HWND, BOOL &) {
	m_SearchBar.GetDlgItem(IDC_SEARCH).SetFocus();
	return 0;
}

LRESULT CView::OnDoSearch(UINT, WPARAM, LPARAM lParam, BOOL &) {
	CString text((PCWSTR)lParam);
	text.MakeLower();
	if (text.IsEmpty())
		m_VisiblePackages = m_AllPackages;
	else {
		m_VisiblePackages.clear();
		for (auto p : m_AllPackages) {
			if (p->Name.MakeLower().Find(text) >= 0)
				m_VisiblePackages.push_back(p);
		}
	}
	if (m_SortColumn >= 0)
		DoSort();

	m_List.SetItemCount(static_cast<int>(m_VisiblePackages.size()));
	return 0;
}

LRESULT CView::OnExecute(WORD, WORD, HWND, BOOL &) {
	auto index = m_List.GetSelectedIndex();
	ATLASSERT(index >= 0);

	auto& item = GetItem(index);
	if (!RunApp(item.FullName))
		MessageBox(L"Failed to launch application", L"Metro Manager");

	return 0;
}

LRESULT CView::OnEditCopy(WORD, WORD, HWND, BOOL &) {
	auto selected = m_List.GetSelectedIndex();
	ATLASSERT(selected >= 0);

	CString text;
	for (int i = 0; i < 6; i++) {
		CString temp;
		m_List.GetItemText(selected, i, temp);
		text += temp;
		if (i < 4)
			text += L",";
	}

	if (OpenClipboard()) {
		::EmptyClipboard();
		auto size = (text.GetLength() + 1) * sizeof(WCHAR);
		auto hData = ::GlobalAlloc(GMEM_MOVEABLE, size);
		if (hData) {
			auto p = ::GlobalLock(hData);
			if (p) {
				::memcpy(p, text.GetBuffer(), size);
				::GlobalUnlock(p);
				::SetClipboardData(CF_UNICODETEXT, hData);
			}
		}
		::CloseClipboard();
	}
	return 0;
}

LRESULT CView::OnColumnClick(int, LPNMHDR hdr, BOOL &) {
	auto lv = (NMLISTVIEW*)hdr;
	auto col = lv->iSubItem;
	auto oldSortColumn = m_SortColumn;
	if (col == m_SortColumn)
		m_SortAscending = !m_SortAscending;
	else {
		m_SortColumn = col;
		m_SortAscending = true;
	}

	HDITEM h;
	h.mask = HDI_FORMAT;
	auto header = m_List.GetHeader();
	header.GetItem(m_SortColumn, &h);
	h.fmt = (h.fmt & HDF_JUSTIFYMASK) | HDF_STRING | (m_SortAscending ? HDF_SORTUP : HDF_SORTDOWN);
	header.SetItem(m_SortColumn, &h);

	if (oldSortColumn >= 0 && oldSortColumn != m_SortColumn) {
		h.mask = HDI_FORMAT;
		header.GetItem(oldSortColumn, &h);
		h.fmt = (h.fmt & HDF_JUSTIFYMASK) | HDF_STRING;
		header.SetItem(oldSortColumn, &h);
	}

	DoSort();
	m_List.RedrawItems(m_List.GetTopIndex(), m_List.GetTopIndex() + m_List.GetCountPerPage());
	return 0;
}

LRESULT CView::OnFindItem(int, LPNMHDR hdr, BOOL &) {
	auto fi = (NMLVFINDITEM*)hdr;

	auto count = m_List.GetItemCount();
	auto str = fi->lvfi.psz;
	auto len = ::wcslen(str);
	for (int i = fi->iStart; i < fi->iStart + count; i++) {
		auto& item = GetItem(i % count);
		if (::_wcsnicmp(item.Name, str, len) == 0)
			return i % count;
	}

	return -1;
}

