// HookInject.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <windows.h>
#include <assert.h>
#include <TlHelp32.h>
#include <Psapi.h>

int Error(const char* text) {
	printf("%s (%d)\n", text, ::GetLastError());
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
					break;
				}
			}
			::CloseHandle(hProcess);
		}
	} while (::Thread32Next(hSnapshot, &th32));
	::CloseHandle(hSnapshot);
	return tid;
}

int main() {
	DWORD tid = FindMainNotepadThread();
	if (tid == 0)
		return Error("Failed to locate Notepad");

	auto hDll = ::LoadLibrary(L"InjectedLib.dll");
	if (!hDll)
		return Error("Failed to locate Dll\n");

	auto setNotify = (void (WINAPI*)(DWORD, HHOOK))::GetProcAddress(hDll, "SetNotificationThread");
	auto hHook = ::SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)::GetProcAddress(hDll, "HookFunction"), hDll, tid);
	if (!hHook)
		return Error("Failed to install hook");

	setNotify(::GetCurrentThreadId(), hHook);
	::PostThreadMessage(tid, WM_NULL, 0, 0);

	MSG msg;
	while (::GetMessage(&msg, nullptr, 0, 0)) {
		if (msg.message == WM_APP) {
			//printf("WM_CHAR: %d (%c)\n", (int)msg.wParam, (char)msg.wParam);
			printf("%c", (int)msg.wParam);
			if (msg.wParam == 13)
				printf("\n");
		}
	}
	::UnhookWindowsHookEx(hHook);
	::FreeLibrary(hDll);
}
