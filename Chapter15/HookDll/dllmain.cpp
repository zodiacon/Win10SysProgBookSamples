// dllmain.cpp : Defines the entry point for the DLL application.

#include "pch.h"

#pragma data_seg(".shared")
DWORD g_ThreadId = 0;
HHOOK g_hHook = nullptr;
#pragma data_seg()
#pragma comment(linker, "/section:.shared,RWS")

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, PVOID pReserved) {
	switch (reason) {
		case DLL_PROCESS_ATTACH:
			::DisableThreadLibraryCalls(hModule);
			break;
		case DLL_PROCESS_DETACH:
			::PostThreadMessage(g_ThreadId, WM_QUIT, 0, 0);
			break;

	}
	return TRUE;
}

extern "C" LRESULT CALLBACK HookFunction(int code, WPARAM wParam, LPARAM lParam) {
	if (code == HC_ACTION) {
		auto msg = (MSG*)lParam;
		if (msg->message == WM_CHAR) {
			::PostThreadMessage(g_ThreadId, WM_APP, msg->wParam, msg->lParam);
			// prevent A characters from getting to the app
			//if (msg->wParam == 'A' || msg->wParam == 'a')
			//	msg->wParam = 0;
		}
	}
	return ::CallNextHookEx(g_hHook, code, wParam, lParam);
}

extern "C" void WINAPI SetNotificationThread(DWORD threadId, HHOOK hHook) {
	g_ThreadId = threadId;
	g_hHook = hHook;
}
