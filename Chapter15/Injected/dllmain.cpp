// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, PVOID lpReserved) {
	switch (reason) {
		case DLL_PROCESS_ATTACH:
			wchar_t text[128];
			::StringCchPrintf(text, _countof(text), L"Injected into process %u", ::GetCurrentProcessId());
			::MessageBox(nullptr, text, L"Injected.Dll", MB_OK);
			break;

		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

