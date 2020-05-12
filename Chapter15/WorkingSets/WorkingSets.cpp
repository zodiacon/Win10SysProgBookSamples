// WorkingSets.cpp : main source file for WorkingSets.exe
//

#include "stdafx.h"
#include "resource.h"
#include "MainFrm.h"
#include "..\IATHelper\IATHelper.h"

decltype(::GetSysColor)* GetSysColorOrg;

COLORREF WINAPI GetSysColorHooked(int index) {
	switch (index) {
		case COLOR_BTNTEXT:
			return RGB(0, 128, 0);

		case COLOR_WINDOWTEXT:
			return RGB(0, 0, 255);
	}

	return GetSysColorOrg(index);
	//return ((decltype(::GetSysColor)*)::GetProcAddress(GetModuleHandle(L"user32"), "GetSysColor"))(index);
}

void HookFunctions() {
	auto hUser32 = ::GetModuleHandle(L"user32");
	// save original functions
	GetSysColorOrg = (decltype(GetSysColorOrg))::GetProcAddress(hUser32, "GetSysColor");

	auto count = IATHelper::HookAllModules("user32.dll", GetSysColorOrg, GetSysColorHooked);
	ATLTRACE(L"Hooked %d calls to GetSysColor\n");
}

CAppModule _Module;

int Run(LPTSTR /*lpstrCmdLine*/ = nullptr, int nCmdShow = SW_SHOWDEFAULT) {
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainFrame wndMain;

	if (wndMain.CreateEx() == nullptr) {
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}

	wndMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow) {
	HRESULT hRes = ::CoInitialize(nullptr);
	ATLASSERT(SUCCEEDED(hRes));

	HookFunctions();

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(nullptr, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
