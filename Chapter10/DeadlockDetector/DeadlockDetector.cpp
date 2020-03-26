// DeadlockDetector.cpp : main source file for DeadlockDetector.exe
//

#include "stdafx.h"
#include "resource.h"
#include "MainDlg.h"

CAppModule _Module;

bool EnableDebugPrivilege() {
	HANDLE hToken;
	if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
		return false;

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!::LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &tp.Privileges[0].Luid))
		return false;

	auto success = ::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
	auto error = ::GetLastError();
	::CloseHandle(hToken);

	return success && error == ERROR_SUCCESS;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpstrCmdLine*/, int /*nCmdShow*/) {
	HRESULT hRes = ::CoInitialize(nullptr);
	ATLASSERT(SUCCEEDED(hRes));

	EnableDebugPrivilege();

	AtlInitCommonControls(ICC_BAR_CLASSES | ICC_TREEVIEW_CLASSES);

	hRes = _Module.Init(nullptr, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = 0;
	{
		CMainDlg dlgMain;
		nRet = (int)dlgMain.DoModal();
	}

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
