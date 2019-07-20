// SingleInstance.cpp : main source file for SingleInstance.exe
//

#include "stdafx.h"
#include "resource.h"
#include "MainDlg.h"

void NotifyOtherInstance();

CAppModule _Module;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpstrCmdLine*/, int /*nCmdShow*/) {
	HANDLE hMutex = ::CreateMutex(nullptr, FALSE, L"SingleInstanceMutex");
	if (!hMutex) {
		CString text;
		text.Format(L"Failed to create mutex (Error: %d)", ::GetLastError());
		::MessageBox(nullptr, text, L"Single Instance", MB_OK);
		return 0;
	}

	if (::GetLastError() == ERROR_ALREADY_EXISTS) {
		NotifyOtherInstance();
		return 0;
	}

	HRESULT hRes = ::CoInitialize(nullptr);
	ATLASSERT(SUCCEEDED(hRes));

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(nullptr, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	{
		CMainDlg dlgMain;
		dlgMain.DoModal();
	}

	::CloseHandle(hMutex);

	_Module.Term();
	::CoUninitialize();

	return 0;
}

void NotifyOtherInstance() {
	auto hWnd = ::FindWindow(nullptr, L"Single Instance");
	if (!hWnd) {
		::MessageBox(nullptr, L"Failed to locate other instance window", L"Single Instance", MB_OK);
		return;
	}

	::PostMessage(hWnd, WM_NOTIFY_INSTANCE, ::GetCurrentProcessId(), 0);
	::ShowWindow(hWnd, SW_NORMAL);
	::SetForegroundWindow(hWnd);
}
