// View.h : interface of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "SearchBar.h"

struct AppItem {
	CString Name, Publisher, InstalledLocation, FullName;
	winrt::Windows::ApplicationModel::PackageVersion Version;
	winrt::Windows::Foundation::DateTime InstalledDate;
	bool IsFramework;
};

class CView : public CWindowImpl<CView> {
public:
	DECLARE_WND_CLASS(NULL)

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_USER + 100, OnDoSearch)
		NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnGetDispInfo)
		NOTIFY_CODE_HANDLER(LVN_COLUMNCLICK, OnColumnClick)
		NOTIFY_CODE_HANDLER(LVN_ODFINDITEM, OnFindItem)

	ALT_MSG_MAP(1)
		COMMAND_ID_HANDLER(ID_EDIT_FIND, OnFind)
		COMMAND_ID_HANDLER(ID_ACTION_EXECUTE, OnExecute)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCopy)
	END_MSG_MAP()

	bool CanExecuteApp() const;
	bool CanCopy() const;

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	const AppItem& GetItem(int index) const;
	CString VersionToString(const winrt::Windows::ApplicationModel::PackageVersion&);
	bool RunApp(PCWSTR fullPackageName);
	void DoSort();
	bool CompareItems(const AppItem& p1, const AppItem& p2) const;
	bool CompareStrings(const wchar_t* s1, const wchar_t* s2) const;
	bool CompareVersions(const winrt::Windows::ApplicationModel::PackageVersion& p1, const winrt::Windows::ApplicationModel::PackageVersion& p2) const;

private:
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnGetDispInfo(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDoSearch(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnExecute(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnColumnClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnFindItem(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	CUpdateUIBase* m_pUpdateUI;
	CSearchBar m_SearchBar;
	CListViewCtrl m_List;
	std::vector<std::shared_ptr<AppItem>> m_AllPackages;
	std::vector<std::shared_ptr<AppItem>> m_VisiblePackages;
	int m_SortColumn = -1;
	bool m_SortAscending;
};
