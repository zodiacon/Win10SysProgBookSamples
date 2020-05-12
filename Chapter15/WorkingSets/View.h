#pragma once

#include "VirtualListView.h"

class CView : 
	public CWindowImpl<CView, CListViewCtrl>,
	public CVirtualListView<CView> {
public:
	DECLARE_WND_SUPERCLASS(nullptr, CListViewCtrl::GetWndClassName())

	CView(CUpdateUIBase* ui);

	BOOL PreTranslateMessage(MSG* pMsg);
	void Refresh();

	void DoSort(const SortInfo*);
	CString GetColumnText(HWND hWnd, int row, int column) const;

	BEGIN_MSG_MAP(CView)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
		CHAIN_MSG_MAP_ALT(CVirtualListView<CView>, 1)
		DEFAULT_REFLECTION_HANDLER()
	ALT_MSG_MAP(1)
		COMMAND_ID_HANDLER(ID_VIEW_REFRESH, OnViewRefresh)
		COMMAND_ID_HANDLER(ID_VIEW_SHOWONLYACCESSIBLEPROCESSES, OnShowAccessibleProcesses)
		COMMAND_ID_HANDLER(ID_PROCESS_EMPTYWORKINGSET, OnEmptyWorkingSet)
	END_MSG_MAP()

private:
	static CString FormatSize(SIZE_T size);
	static CString WSFlagsToString(DWORD flags);

	struct ProcessInfo {
		DWORD Id;
		CString ImageName;
		SIZE_T MinWorkingSet, MaxWorkingSet;
		DWORD WorkingSetFlags;
		PROCESS_MEMORY_COUNTERS_EX Counters;
		bool CountersAvailable{ false };
	};

	static bool CompareItems(const ProcessInfo& p1, const ProcessInfo& p2, int col, bool asc);

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnViewRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnShowAccessibleProcesses(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEmptyWorkingSet(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	std::vector<ProcessInfo> m_Items;
	CUpdateUIBase* m_UI;
	bool m_ShowOnlyAccessibleProcesses{ false };
};
