// View.cpp : implementation of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "EventParser.h"
#include "View.h"
#include "..\ThreadingHelpers\AutoReaderWriterLock.h"
#include "..\HashCalc\MD5Calculator.h"

BOOL CView::PreTranslateMessage(MSG* pMsg) {
	return FALSE;
}

void CView::ToggleUseCache(CUpdateUIBase& ui) {
	m_UseCache = !m_UseCache;
	ui.UISetCheck(ID_CALCULATE_USECACHE, m_UseCache);
}

void CView::OnEvent(PEVENT_RECORD record) {
	EventParser parser(record);
	if (parser.GetEventHeader().EventDescriptor.Opcode != 10)	// ID 10 is a load event
		return;

	auto fileName = parser.GetProperty(L"FileName");
	if (fileName) {
		EventData data;
		data.FileName = parser.GetDosNameFromNtName(fileName->GetUnicodeString()).c_str();
		data.ProcessId = parser.GetProcessId();
		data.Time = parser.GetEventHeader().TimeStamp.QuadPart;
		data.CalcDone = false;
		size_t size;
		{
			AutoReaderWriterLockExclusive locker(m_EventsLock);
			m_Events.push_back(std::move(data));
			size = m_Events.size();
		}
		int index = static_cast<int>(size - 1);

		// initiate work from the UI thread

		PostMessage(WM_START_CALC, index, size);
	}
}

void CView::Clear() {
	AutoReaderWriterLockExclusive locker(m_EventsLock);
	m_Events.clear();
	SetItemCount(0);
}

DWORD CView::DoCalc(int index) {
	LARGE_INTEGER pcStart;
	::QueryPerformanceCounter(&pcStart);

	static LARGE_INTEGER freq;
	if (freq.QuadPart == 0)
		::QueryPerformanceFrequency(&freq);

	EventData data;
	{
		// get data item
		AutoReaderWriterLockShared locker(m_EventsLock);
		data = m_Events[index];
	}

	::SetThreadPriority(::GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
	auto hash = MD5Calculator::Calculate(data.FileName);
	{
		AutoReaderWriterLockShared locker(m_EventsLock);
		auto& data = m_Events[index];
		data.CalcDone = true;
		data.MD5Hash = hash;
		data.Cached = false;
		data.CalculatingThreadId = ::GetCurrentThreadId();
		LARGE_INTEGER pcEnd;
		::QueryPerformanceCounter(&pcEnd);
		data.CalculationTime = (DWORD)((pcEnd.QuadPart - pcStart.QuadPart) * 1000000 / freq.QuadPart);
	}
	if (m_UseCache)
		m_Cache.Add(data.FileName, hash);

	::SetThreadPriority(::GetCurrentThread(), THREAD_MODE_BACKGROUND_END);

	SendMessage(LVM_REDRAWITEMS, index, index);
	return 0;
}

CString CView::FormatTime(ULONGLONG time) {
	CTime t(*(FILETIME*)&time);
	auto text = t.Format(L"%x %X");
	text.Format(L"%s.%06d", (PCWSTR)text, time / 100 % 1000000);
	return text;
}

CString CView::GetProcessName(DWORD pid) const {
	wil::unique_handle hProcess(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
	if (hProcess) {
		WCHAR name[MAX_PATH];
		DWORD size = MAX_PATH;
		if (::QueryFullProcessImageName(hProcess.get(), 0, name, &size)) {
			auto bs = ::wcsrchr(name, L'\\');
			return bs ? bs + 1 : name;
		}
	}
	return L"";
}

LRESULT CView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	DefWindowProc();

	SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT);

	CImageList images;
	images.Create(16, 16, ILC_COLOR32, 4, 4);
	SetImageList(images, LVSIL_SMALL);

	InsertColumn(0, L"Time", LVCFMT_LEFT, 180);
	InsertColumn(1, L"Process Id", LVCFMT_RIGHT, 80);
	InsertColumn(2, L"Process Name", LVCFMT_LEFT, 150);
	InsertColumn(3, L"Image Name", LVCFMT_LEFT, 450);
	InsertColumn(4, L"MD5 Hash", LVCFMT_RIGHT, 240);
	InsertColumn(5, L"Calculating TID", LVCFMT_RIGHT, 90);
	InsertColumn(6, L"Calculation Time", LVCFMT_RIGHT, 90);
	InsertColumn(7, L"Cached?", LVCFMT_LEFT, 70);

	return 0;
}

LRESULT CView::OnStartCalc(UINT, WPARAM index, LPARAM size, BOOL&) {
	SetItemCountEx(static_cast<int>(size), LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);

	// check if data already in cache
	if (m_UseCache) {
		Hash hash;
		{
			AutoReaderWriterLockShared lokcer(m_EventsLock);
			auto& data = m_Events[index];
			hash = m_Cache.Get(data.FileName);
			if (!hash.empty()) {
				data.Cached = true;
				data.MD5Hash = hash;
				data.CalculatingThreadId = 0;
				data.CalcDone = true;
			}
		}
		if (!hash.empty()) {
			SendMessage(LVM_REDRAWITEMS, index, index);
			return 0;
		}
	}

	auto data = new CalcThreadData;
	data->View = this;
	data->Index = (int)index;

	// options 1: use TrySubmitThreadpoolCallback
	//if (!::TrySubmitThreadpoolCallback([](auto instance, auto param) {
	//	auto data = (CalcThreadData*)param;
	//	auto view = data->View;
	//	auto index = data->Index;
	//	delete data;
	//	view->DoCalc(index);
	//	}, data, nullptr)) {
	//	AtlMessageBox(nullptr, L"Failed to submit thread pool work!", IDR_MAINFRAME, MB_ICONERROR);
	//	return 0;
	//}

	// option 2: use a manually created work item
	wil::unique_threadpool_work_nowait work(::CreateThreadpoolWork([](auto instance, auto param, auto work) {
		auto data = (CalcThreadData*)param;
		auto view = data->View;
		auto index = data->Index;
		delete data;
		view->DoCalc(index);
		}, data, nullptr));
	if(!work) {
		AtlMessageBox(nullptr, L"Failed to submit thread pool work!", IDR_MAINFRAME, MB_ICONERROR);
		return 0;
	}
	::SubmitThreadpoolWork(work.get());

	return 0;
}

LRESULT CView::OnGetDispInfo(int, LPNMHDR hdr, BOOL&) {
	auto di = reinterpret_cast<NMLVDISPINFO*>(hdr);
	auto& item = di->item;
	auto index = item.iItem;
	auto col = item.iSubItem;
	EventData data;
	{
		AutoReaderWriterLockShared locker(m_EventsLock);
		data = m_Events[index];
	}

	if (di->item.mask & LVIF_TEXT) {
		switch (col) {
		case 0:	// time
			::StringCchCopy(item.pszText, item.cchTextMax, FormatTime(data.Time));
			break;

		case 1:	// PID
			::StringCchPrintf(item.pszText, item.cchTextMax, L"%d", data.ProcessId);
			break;

		case 2: // process name
			::StringCchCopy(item.pszText, item.cchTextMax, GetProcessName(data.ProcessId));
			break;

		case 3:	// file name
			::StringCchCopy(item.pszText, item.cchTextMax, data.FileName);
			break;

		case 4:	// hash
			if (data.CalcDone) {
				CString hash;
				for (auto b : data.MD5Hash)
					hash.Format(L"%s%02X", (PCWSTR)hash, b);
				::StringCchCopy(item.pszText, item.cchTextMax, hash);
			}
			break;

		case 5:
			if (data.CalcDone && !data.Cached) {
				::StringCchPrintf(item.pszText, item.cchTextMax, L"%d", data.CalculatingThreadId);
			}
			break;

		case 6:
			if (data.CalcDone && data.CalculatingThreadId)
				::StringCchPrintf(item.pszText, item.cchTextMax, L"%d usec", data.CalculationTime);
			break;

		case 7:
			if (data.CalcDone)
				item.pszText = data.Cached ? L"Yes" : L"No";
			break;
		}
	}
	return 0;
}
