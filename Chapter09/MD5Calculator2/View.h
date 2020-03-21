// View.h : interface of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "..\ThreadingHelpers\ReaderWriterLock.h"
#include "HashCache.h"

struct EventData {
	CString FileName;
	ULONGLONG Time;
	DWORD ProcessId;
	Hash MD5Hash;
	DWORD CalculatingThreadId;
	DWORD CalculationTime;
	bool Cached : 1;
	bool CalcDone : 1;
};

#define WM_START_CALC (WM_APP+1)

class CView : public CWindowImpl<CView, CListViewCtrl> {
public:
	DECLARE_WND_SUPERCLASS(nullptr, CListViewCtrl::GetWndClassName())

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CView)
		MESSAGE_HANDLER(WM_START_CALC, OnStartCalc)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnGetDispInfo)
	END_MSG_MAP()

	void ToggleUseCache(CUpdateUIBase& ui);

	void OnEvent(PEVENT_RECORD record);
	void Clear();
	DWORD DoCalc(int index);
	static CString FormatTime(ULONGLONG time);
	CString GetProcessName(DWORD pid) const;

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnStartCalc(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnGetDispInfo(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	struct CalcThreadData {
		CView* View;
		int Index;
	};

private:
	std::vector<EventData> m_Events;
	HashCache m_Cache;
	ReaderWriterLock m_EventsLock;
	wil::unique_threadpool_work m_Work;
	bool m_UseCache{ false };
};
