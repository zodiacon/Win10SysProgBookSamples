// BasicSharing.cpp : main source file for BasicSharing.exe
//

#include "stdafx.h"

#include "resource.h"
#include "MainDlg.h"
#include <detours.h>
#include <strsafe.h>

CAppModule _Module;

decltype(::GetWindowTextW)* GetWindowTextOrg = ::GetWindowTextW;
decltype(::GetWindowTextLengthW)* GetWindowTextLengthOrg = ::GetWindowTextLengthW;

static WCHAR extra[] = L" (Hooked!)";

bool IsEditControl(HWND hWnd) {
	WCHAR name[32];
	return ::GetClassName(hWnd, name, _countof(name)) && ::_wcsicmp(name, L"EDIT") == 0;
}

int WINAPI GetWindowTextHooked(
	_In_  HWND   hWnd,
	_Out_ LPWSTR lpString,
	_In_  int    nMaxCount) {

	auto count = GetWindowTextOrg(hWnd, lpString, nMaxCount);

	if (IsEditControl(hWnd)) {
		if (count + _countof(extra) <= nMaxCount) {
			::StringCchCatW(lpString, nMaxCount, extra);
			count += _countof(extra);
		}
	}
	return count;
}

int WINAPI GetWindowTextLengthHooked(HWND hWnd) {
	auto len = GetWindowTextLengthOrg(hWnd);
	if(IsEditControl(hWnd))
		len += (int)wcslen(extra);
	return len;
}

bool HookFunctions() {
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((PVOID*)&GetWindowTextOrg, GetWindowTextHooked);
	DetourAttach((PVOID*)&GetWindowTextLengthOrg, GetWindowTextLengthHooked);
	auto error = DetourTransactionCommit();
	return error == ERROR_SUCCESS;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpstrCmdLine*/, int /*nCmdShow*/) {
	HRESULT hRes = ::CoInitialize(nullptr);
	ATLASSERT(SUCCEEDED(hRes));

	HookFunctions();

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(nullptr, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	{
		CMainDlg dlgMain;
		dlgMain.DoModal();
	}

	_Module.Term();
	::CoUninitialize();

	return 0;
}
