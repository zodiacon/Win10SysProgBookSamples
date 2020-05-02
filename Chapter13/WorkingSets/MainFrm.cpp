// MainFrm.cpp : implementation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "MainFrm.h"

CMainFrame::CMainFrame() : m_view(this) {
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) {
	if (CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;

	return m_view.PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle() {
	return FALSE;
}

bool CMainFrame::IsRunningElevated() {
	wil::unique_handle hToken;
	if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, hToken.addressof()))
		return false;

	TOKEN_ELEVATION te;
	DWORD len;
	if (::GetTokenInformation(hToken.get(), TokenElevation, &te, sizeof(te), &len))
		return te.TokenIsElevated ? true : false;
	return false;
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	m_hWndClient = m_view.Create(m_hWnd, rcDefault, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | 
		LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_OWNERDATA, WS_EX_CLIENTEDGE);

	CMenuHandle menu(GetMenu());
	if (IsRunningElevated()) {
		menu.GetSubMenu(0).RemoveMenu(ID_FILE_RUNASADMINISTRATOR, MF_BYCOMMAND);
		menu.GetSubMenu(0).DeleteMenu(0, MF_BYPOSITION);
		DrawMenuBar();

		CString text;
		GetWindowText(text);
		SetWindowText(text + L" (Administrator)");
	}

	// register object for message filtering and idle updates
	auto pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddMenu(GetMenu());

	UIEnable(ID_PROCESS_EMPTYWORKINGSET, false);

	return 0;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	// unregister message filtering and idle updates
	auto pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	bHandled = FALSE;
	return 1;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnRunAsAdmin(WORD, WORD, HWND, BOOL&) {
	WCHAR path[MAX_PATH];
	::GetModuleFileName(nullptr, path, _countof(path));
	SHELLEXECUTEINFO shi = { sizeof(shi) };
	shi.lpFile = path;
	shi.nShow = SW_SHOWDEFAULT;
	shi.lpVerb = L"runas";
	shi.fMask = SEE_MASK_NOASYNC;
	auto ok = ::ShellExecuteEx(&shi);
	if (ok) {
		PostMessage(WM_CLOSE);
	}

	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD, WORD, HWND, BOOL&) {
	AtlMessageBox(*this, L"Working Sets v1.0 (C)2020", IDR_MAINFRAME, MB_ICONINFORMATION);
	return 0;
}

