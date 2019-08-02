// ProcList.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

int Error(const char* text) {
	printf("%s (%d)\n", text, ::GetLastError());
	return 1;
}

int main() {
	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
		return Error("Failed to create snapshot");

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);

	if (!::Process32First(hSnapshot, &pe))
		return Error("Failed in Process32First");

	do {
		printf("PID:%6d (PPID:%6d): %ws (Threads=%d) (Priority=%d)\n",
			pe.th32ProcessID, pe.th32ParentProcessID, pe.szExeFile, pe.cntThreads, pe.pcPriClassBase);
	} while (::Process32Next(hSnapshot, &pe));

	::CloseHandle(hSnapshot);

	return 0;
}
