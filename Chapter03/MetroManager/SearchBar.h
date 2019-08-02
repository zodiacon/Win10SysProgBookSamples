#pragma once

#include "resource.h"

class CSearchBar : 
	public CDialogImpl<CSearchBar> {
public:
	enum { IDD = IDD_SEARCHBAR };

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CSearchBar)
		COMMAND_ID_HANDLER(IDC_DOSEARCH, OnDoSearch)
		COMMAND_ID_HANDLER(IDC_CLEAR, OnClear)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnControlColor)
	END_MSG_MAP()

private:
	LRESULT OnDoSearch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnClear(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnControlColor(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

};

