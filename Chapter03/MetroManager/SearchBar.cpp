#include "pch.h"
#include "SearchBar.h"

BOOL CSearchBar::PreTranslateMessage(MSG * pMsg) {
	return CWindow::IsDialogMessage(pMsg);
}

LRESULT CSearchBar::OnDoSearch(WORD, WORD wID, HWND, BOOL &) {
	CString text;
	GetDlgItemText(IDC_SEARCH, text);
	::SendMessage(GetParent(), WM_USER + 100, 0, (LPARAM)(PCWSTR)text);

	return 0;
}

LRESULT CSearchBar::OnClear(WORD, WORD wID, HWND, BOOL &) {
	SetDlgItemText(IDC_SEARCH, L"");
	::SendMessage(GetParent(), WM_USER + 100, 0, (LPARAM)L"");

	return 0;
}

LRESULT CSearchBar::OnEraseBackground(UINT, WPARAM wParam, LPARAM, BOOL &) {
	CDCHandle dc((HDC)wParam);
	RECT rc;
	GetClientRect(&rc);
	dc.FillSolidRect(&rc, ::GetSysColor(COLOR_WINDOW));

	return 1;
}

LRESULT CSearchBar::OnControlColor(UINT, WPARAM, LPARAM, BOOL &) {
	return COLOR_WINDOW + 1;
}
