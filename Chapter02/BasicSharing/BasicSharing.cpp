// BasicSharing.cpp : main source file for BasicSharing.exe
//

#include "stdafx.h"

#include "resource.h"
#include "MainDlg.h"

CAppModule _Module;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpstrCmdLine*/, int /*nCmdShow*/) {
	HRESULT hRes = ::CoInitialize(nullptr);
	ATLASSERT(SUCCEEDED(hRes));

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
