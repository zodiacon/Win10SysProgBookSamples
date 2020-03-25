// breakin.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>

int Error(const char* text) {
	printf("%s (%d)\n", text, ::GetLastError());
	return 1;
}

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		printf("Usage: breakin <pid>\n");
		return 0;
	}

	int pid = atoi(argv[1]);

	auto hProcess = ::OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
		PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
		FALSE, pid);
	if (!hProcess)
		return Error("Failed to open process");

	auto hThread = ::CreateRemoteThread(hProcess, nullptr, 0,
		(LPTHREAD_START_ROUTINE)::GetProcAddress(::GetModuleHandle(L"kernel32"), "DebugBreak"),
		nullptr, 0, nullptr);
	if (!hThread)
		return Error("Failed to create remote thread");

	printf("Remote thread created successfully!\n");

	::CloseHandle(hThread);
	::CloseHandle(hProcess);

	return 0;
}

