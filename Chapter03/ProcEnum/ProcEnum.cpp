// ProcEnum.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

bool EnableDebugPrivilege() {
	wil::unique_handle hToken;
	if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, hToken.addressof()))
		return false;

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!::LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &tp.Privileges[0].Luid))
		return false;

	if (!::AdjustTokenPrivileges(hToken.get(), FALSE, &tp, sizeof(tp), nullptr, nullptr))
		return false;

	return ::GetLastError() == ERROR_SUCCESS;
}

int main() {
	if (!EnableDebugPrivilege()) {
		printf("Failed to enable Debug privilege!\n");
	}

	int maxCount = 256;
	std::unique_ptr<DWORD[]> pids;
	int count = 0;

	for (;;) {
		pids = std::make_unique<DWORD[]>(maxCount);
		DWORD actualSize;
		if (!::EnumProcesses(pids.get(), maxCount * sizeof(DWORD), &actualSize))
			break;

		count = actualSize / sizeof(DWORD);
		if (count < maxCount)
			break;

		// need to resize
		maxCount *= 2;
	}

	for (int i = 0; i < count; i++) {
		DWORD pid = pids[i];
		HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
		if (!hProcess) {
			printf("Failed to get a handle to process %d (error=%d)\n", pid, ::GetLastError());
			continue;
		}
		FILETIME start = { 0 }, dummy;
		if (!::GetProcessTimes(hProcess, &start, &dummy, &dummy, &dummy)) {
			printf("Failed!!!\n");
			continue;
		}

		SYSTEMTIME st;
		::FileTimeToLocalFileTime(&start, &start);
		::FileTimeToSystemTime(&start, &st);
		WCHAR exeName[MAX_PATH];
		DWORD size = MAX_PATH;
		DWORD count = ::QueryFullProcessImageName(hProcess, 0, exeName, &size);
		printf("PID: %5d, Start: %d/%d/%d %02d:%02d:%02d Image: %ws\n",
			pid, st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond, count > 0 ? exeName : L"Unknown");
	}

	return 0;
}
