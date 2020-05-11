// Injector.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>

int Error(const char* msg) {
	printf("%s (%u)\n", msg, ::GetLastError());
	return 1;
}

int main(int argc, const char* argv[]) {
	if (argc < 3) {
		printf("Usage: injector <pid> <dllpath>\n");
		return 0;
	}

	HANDLE hProcess = ::OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_CREATE_THREAD,
		FALSE, atoi(argv[1]));
	if (!hProcess)
		return Error("Failed to open process");

	void* buffer = ::VirtualAllocEx(hProcess, nullptr, 1 << 12, 
		MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!buffer)
		return Error("Failed to allocate buffer in target process");

	if (!::WriteProcessMemory(hProcess, buffer, argv[2], ::strlen(argv[2]) + 1, nullptr))
		return Error("Failed to write to target process");

	DWORD tid;
	HANDLE hThread = ::CreateRemoteThread(hProcess, nullptr, 0, 
		(LPTHREAD_START_ROUTINE)::GetProcAddress(::GetModuleHandle(L"kernel32"), "LoadLibraryA"), 
		buffer, 0, &tid);
	if (!hThread)
		return Error("Failed to create remote thread");

	printf("Thread %u created successfully!\n", tid);
	if (WAIT_OBJECT_0 == ::WaitForSingleObject(hThread, 5000))
		printf("Thread exited.\n");
	else
		printf("Thread still hanging around...\n");
	
	// be nice
	::VirtualFreeEx(hProcess, buffer, 0, MEM_RELEASE);

	::CloseHandle(hThread);
	::CloseHandle(hProcess);
	
	return 0;
}

