// HookInject.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <windows.h>
#include <assert.h>
#include <TlHelp32.h>
#include <Psapi.h>

int Error(const char* text) {
	printf("%s (%u)\n", text, ::GetLastError());
	return 1;
}

DWORD FindMainNotepadThread() {
	auto hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
		return 0;

	DWORD tid = 0;
	THREADENTRY32 th32;
	th32.dwSize = sizeof(th32);

	::Thread32First(hSnapshot, &th32);
	do {
		auto hProcess = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, th32.th32OwnerProcessID);
		if (hProcess) {
			WCHAR name[MAX_PATH];
			if (::GetProcessImageFileName(hProcess, name, MAX_PATH) > 0) {
				auto bs = ::wcsrchr(name, L'\\');
				if (bs && ::_wcsicmp(bs, L"\\notepad.exe") == 0) {
					tid = th32.th32ThreadID;
				}
			}
			::CloseHandle(hProcess);
		}
	} while (tid == 0 && ::Thread32Next(hSnapshot, &th32));
	::CloseHandle(hSnapshot);

	return tid;
}

int main() {
	DWORD tid = FindMainNotepadThread();
	if (tid == 0)
		return Error("Failed to locate Notepad");

	auto hDll = ::LoadLibrary(L"HookDll");
	if (!hDll)
		return Error("Failed to locate Dll\n");

	using PSetNotify = void (WINAPI*)(DWORD, HHOOK);
	auto setNotify = (PSetNotify)::GetProcAddress(hDll, "SetNotificationThread");
	if (!setNotify)
		return Error("Failed to locate SetNotificationThread function in DLL");

	auto hookFunc = (HOOKPROC)::GetProcAddress(hDll, "HookFunction");
	if (!hookFunc)
		return Error("Failed to locate HookFunction function in DLL");

	auto hHook = ::SetWindowsHookEx(WH_GETMESSAGE, hookFunc, hDll, tid);
	if (!hHook)
		return Error("Failed to install hook");

	setNotify(::GetCurrentThreadId(), hHook);
	::PostThreadMessage(tid, WM_NULL, 0, 0);

	MSG msg;
	while (::GetMessage(&msg, nullptr, 0, 0)) {
		if (msg.message == WM_APP) {
			printf("%c", (int)msg.wParam);
			if (msg.wParam == 13)
				printf("\n");
		}
	}
	::UnhookWindowsHookEx(hHook);
	::FreeLibrary(hDll);

	return 0;
}
