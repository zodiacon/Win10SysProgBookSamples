// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "MainDlg.h"

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg) {
	if (m_hAccel && ::TranslateAccelerator(*this, m_hAccel, pMsg))
		return TRUE;
	return CWindow::IsDialogMessage(pMsg);
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// center the dialog on the screen
	CenterWindow();

	m_List.Attach(GetDlgItem(IDC_LIST));
	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
	m_Progress.Attach(GetDlgItem(IDC_PROGRESS));

	m_hAccel = AtlLoadAccelerators(IDR_MAINFRAME);

	_Module.GetMessageLoop()->AddMessageFilter(this);

	CImageList images;
	images.Create(16, 16, ILC_COLOR32, 2, 2);
	images.AddIcon(AtlLoadIconImage(IDI_FILE, 0, 16, 16));
	images.AddIcon(AtlLoadIconImage(IDI_FOLDER, 0, 16, 16));
	m_List.SetImageList(images, LVSIL_SMALL);

	m_List.InsertColumn(0, L"Source", LVCFMT_LEFT, 350);
	m_List.InsertColumn(1, L"Size", LVCFMT_RIGHT, 100);
	m_List.InsertColumn(2, L"Destination", LVCFMT_LEFT, 250);

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	UpdateButtons();

	return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT, WPARAM, LPARAM, BOOL&) {
	PostQuitMessage(0);
	return 0;
}

LRESULT CMainDlg::OnGo(WORD, WORD wID, HWND, BOOL&) {
	// transfer list data to vector
	m_Data.clear();
	int count = m_List.GetItemCount();
	m_Data.reserve(count);
	for (int i = 0; i < count; i++) {
		if (m_List.GetItemData(i) != (DWORD_PTR)Type::File) {
			// folders not yet implemented
			continue;
		}

		FileData data;
		m_List.GetItemText(i, 0, data.Src);
		m_List.GetItemText(i, 2, data.Dst);
		m_Data.push_back(std::move(data));
	}

	StartCopy();

	m_Progress.SetPos(0);
	m_Running = true;
	UpdateButtons();

	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (m_Running) {
		AtlMessageBox(*this, L"Copy in progress... cannot close application", IDR_MAINFRAME, MB_ICONWARNING);
		return 0;
	}
	DestroyWindow();
	return 0;
}

LRESULT CMainDlg::OnAddFiles(WORD, WORD wID, HWND, BOOL&) {
	CMultiFileDialog dlg(nullptr, nullptr,
		OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT,
		L"All Files (*.*)\0*.*\0", *this);
	dlg.ResizeFilenameBuffer(1 << 16);

	if (dlg.DoModal() == IDOK) {
		CString path;
		int errors = 0;
		dlg.GetFirstPathName(path);
		do {
			wil::unique_handle hFile(::CreateFile(path, 0, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
			if (!hFile) {
				errors++;
				continue;
			}
			LARGE_INTEGER size;
			::GetFileSizeEx(hFile.get(), &size);
			int n = m_List.AddItem(m_List.GetItemCount(), 0, path, 0);
			m_List.SetItemText(n, 1, FormatSize(size.QuadPart));
			m_List.SetItemData(n, (DWORD_PTR)Type::File);
		} while (dlg.GetNextPathName(path));
		m_List.EnsureVisible(m_List.GetItemCount() - 1, FALSE);
		UpdateButtons();
		if (errors > 0)
			AtlMessageBox(*this, L"Some files failed to open", IDR_MAINFRAME, MB_ICONEXCLAMATION);
	}
	return 0;
}

LRESULT CMainDlg::OnAddDirectory(WORD, WORD wID, HWND, BOOL&) {
	CFolderDialog dlg(*this, L"Select Directory", BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE);
	if (dlg.DoModal() == IDOK) {
		int n = m_List.AddItem(m_List.GetItemCount(), 0, dlg.m_szFolderPath, 1);
		m_List.SetItemData(n, (DWORD_PTR)Type::Folder);
		UpdateButtons();
	}
	return 0;
}

LRESULT CMainDlg::OnSetDestination(WORD, WORD wID, HWND, BOOL&) {
	CFolderDialog dlg(*this, L"Select Destination Directory", BIF_RETURNONLYFSDIRS | BIF_USENEWUI);
	if (dlg.DoModal() == IDOK) {
		auto selected = m_List.GetSelectedCount();
		if (selected == 0) {
			auto count = m_List.GetItemCount();
			for (int i = 0; i < count; i++)
				m_List.SetItemText(i, 2, dlg.m_szFolderPath);
			m_Destinations = count;
		}
		else {
			int index = -1;
			CString text;
			for (;;) {
				index = m_List.GetNextItem(index, LVNI_SELECTED);
				if (index < 0)
					break;
				m_List.GetItemText(index, 2, text);
				m_List.SetItemText(index, 2, dlg.m_szFolderPath);
				if (text.IsEmpty())
					m_Destinations++;
			}
		}
		UpdateButtons();
	}
	return LRESULT();
}

LRESULT CMainDlg::OnSelectAll(WORD, WORD wID, HWND, BOOL&) {
	m_List.SelectAllItems();
	m_List.SetFocus();

	return 0;
}

LRESULT CMainDlg::OnItemChanged(int, LPNMHDR, BOOL&) {
	UpdateButtons();
	return 0;
}

LRESULT CMainDlg::OnRemove(WORD, WORD wID, HWND, BOOL&) {
	ATLASSERT(m_List.GetSelectedCount() > 0);

	int index = -1;
	CString text;
	for (;;) {
		index = m_List.GetNextItem(index, LVNI_SELECTED);
		if (index < 0)
			break;
		m_List.GetItemText(index, 1, text);
		if (!text.IsEmpty())
			m_Destinations--;
		m_List.DeleteItem(index);
		index--;
	}
	return 0;
}

LRESULT CMainDlg::OnProgress(UINT, WPARAM, LPARAM, BOOL&) {
	m_Progress.StepIt();

	return 0;
}

LRESULT CMainDlg::OnProgressStart(UINT, WPARAM wParam, LPARAM, BOOL&) {
	m_Progress.SetStep(1);
	m_Progress.SetRange32(0, static_cast<int>(wParam));

	return 0;
}

LRESULT CMainDlg::OnDone(UINT, WPARAM, LPARAM, BOOL&) {
	m_Data.clear();

	m_Running = false;
	AtlMessageBox(*this, L"All done!", IDR_MAINFRAME, MB_ICONINFORMATION);
	m_Progress.SetPos(0);
	UpdateButtons();

	return 0;
}

void CMainDlg::UpdateButtons() {
	auto count = m_List.GetItemCount();
	GetDlgItem(IDC_GO).EnableWindow(!m_Running && m_Destinations > 0 && m_Destinations == count);
	auto selected = m_List.GetSelectedCount();
	GetDlgItem(IDC_REMOVE).EnableWindow(!m_Running && selected > 0);
	GetDlgItem(IDC_SETDEST).EnableWindow(!m_Running && selected > 0 || (m_Destinations == 0 && count > 0));
	GetDlgItem(IDC_ADDFILES).EnableWindow(!m_Running);
	GetDlgItem(IDC_ADDDIR).EnableWindow(!m_Running);
}

void CMainDlg::StartCopy() {
	m_OperationCount = 0;

	for (auto& data : m_Data) {
		// open source file for async I/O
		wil::unique_handle hSrc(::CreateFile(data.Src, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr));
		if (!hSrc) {
			PostMessage(WM_ERROR, ::GetLastError());
			continue;
		}

		// get file size
		LARGE_INTEGER size;
		::GetFileSizeEx(hSrc.get(), &size);

		// create target file and set final size
		CString filename = data.Src.Mid(data.Src.ReverseFind(L'\\'));
		wil::unique_handle hDst(::CreateFile(data.Dst + filename, GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED, nullptr));
		if (!hDst) {
			PostMessage(WM_ERROR, ::GetLastError());
			continue;
		}

		::SetFilePointerEx(hDst.get(), size, nullptr, FILE_BEGIN);
		::SetEndOfFile(hDst.get());

		data.tpDst.reset(::CreateThreadpoolIo(hDst.get(), WriteCallback, this, nullptr));
		data.tpSrc.reset(::CreateThreadpoolIo(hSrc.get(), ReadCallback, data.tpDst.get(), nullptr));

		data.hSrc = std::move(hSrc);
		data.hDst = std::move(hDst);

		// initiate first read operation
		auto io = new IOData;
		io->Size = size.QuadPart;
		io->Buffer = std::make_unique<BYTE[]>(chunkSize);
		io->hSrc = data.hSrc.get();
		io->hDst = data.hDst.get();
		::ZeroMemory(io, sizeof(OVERLAPPED));
		::StartThreadpoolIo(data.tpSrc.get());
		auto ok = ::ReadFile(io->hSrc, io->Buffer.get(), chunkSize, nullptr, io);
		ATLASSERT(!ok && ::GetLastError() == ERROR_IO_PENDING);
		::InterlockedAdd64(&m_OperationCount, (size.QuadPart + chunkSize - 1) / chunkSize);
	}

	PostMessage(WM_PROGRESS_START, (WPARAM)m_OperationCount);
}

CString CMainDlg::FormatSize(LONGLONG size) {
	CString text;
	if (size < 1 << 16)
		text.Format(L"%u B", (ULONG)size);
	else if (size < 1 << 22)
		text.Format(L"%u KB", size >> 10);
	else if (size < 1LL << 34)
		text.Format(L"%u MB", size >> 20);
	else
		text.Format(L"%u GB", size >> 30);
	return text;
}

void CMainDlg::ReadCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PVOID Overlapped, ULONG IoResult, ULONG_PTR NumberOfBytesTransferred, PTP_IO Io) {
	if (IoResult == ERROR_SUCCESS) {
		auto io = static_cast<IOData*>(Overlapped);
		ULARGE_INTEGER offset = { io->Offset, io->OffsetHigh };
		offset.QuadPart += chunkSize;
		if (offset.QuadPart < io->Size) {
			auto newio = new IOData;
			newio->Size = io->Size;
			newio->Buffer = std::make_unique<BYTE[]>(chunkSize);
			newio->hSrc = io->hSrc;
			newio->hDst = io->hDst;
			::ZeroMemory(newio, sizeof(OVERLAPPED));
			newio->Offset = offset.LowPart;
			newio->OffsetHigh = offset.HighPart;
			::StartThreadpoolIo(Io);
			auto ok = ::ReadFile(newio->hSrc, newio->Buffer.get(), chunkSize, nullptr, newio);
			auto error = ::GetLastError();
			ATLASSERT(!ok && error == ERROR_IO_PENDING);
		}

		// read done, initiate write to the same offset in the target file
		io->Internal = io->InternalHigh = 0;
		auto writeIo = (PTP_IO)Context;
		::StartThreadpoolIo(writeIo);
		auto ok = ::WriteFile(io->hDst, io->Buffer.get(), (ULONG)NumberOfBytesTransferred, nullptr, io);
		auto error = ::GetLastError();
		ATLASSERT(!ok && error == ERROR_IO_PENDING);
	}
}

void CMainDlg::WriteCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PVOID Overlapped, ULONG IoResult, ULONG_PTR NumberOfBytesTransferred, PTP_IO Io) {
	if (IoResult == ERROR_SUCCESS) {
		auto pThis = static_cast<CMainDlg*>(Context);
		pThis->PostMessage(WM_PROGRESS);
		auto io = static_cast<IOData*>(Overlapped);
		delete io;
		if (0 == InterlockedDecrement64(&pThis->m_OperationCount)) {
			pThis->PostMessage(WM_DONE);
		}
	}
}
