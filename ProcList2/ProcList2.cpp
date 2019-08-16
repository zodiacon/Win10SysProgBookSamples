// ProcList2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#include <atltime.h>

#pragma comment(lib, "wtsapi32")

CString GetUserNameFromSid(PSID sid) {
	if (sid == nullptr)
		return L"";
	WCHAR name[128], domain[64];
	DWORD len = _countof(name);
	DWORD domainLen = _countof(domain);
	SID_NAME_USE use;
	if (!::LookupAccountSid(nullptr, sid, name, &len, domain, &domainLen, &use))
		return L"";

	return CString(domain) + L"\\" + name;
}

CString GetCpuTime(PWTS_PROCESS_INFO_EX pi) {
	auto totalTime = pi->KernelTime.QuadPart + pi->UserTime.QuadPart;
	return CTimeSpan(totalTime / 10000000LL).Format(L"%D:%H:%M:%S");
}

bool EnumerateProcesses1() {
	PWTS_PROCESS_INFO info;
	DWORD count;
	if (!::WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &info, &count))
		return false;

	for (DWORD i = 0; i < count; i++) {
		auto pi = info + i;
		printf("\nPID: %5d (S: %d) (User: %ws) %ws", 
			pi->ProcessId, pi->SessionId, 
			(PCWSTR)GetUserNameFromSid(pi->pUserSid), pi->pProcessName);
	}
	::WTSFreeMemory(info);

	return true;
}

bool EnumerateProcesses2() {
	PWTS_PROCESS_INFO_EX info;
	DWORD count;
	DWORD level = 1;
	if (!::WTSEnumerateProcessesEx(WTS_CURRENT_SERVER_HANDLE, &level, WTS_ANY_SESSION, (PWSTR*)&info, &count))
		return false;

	for (DWORD i = 0; i < count; i++) {
		auto pi = info + i;
		printf("\nPID: %5d (S: %d) (T: %3d) (H: %4d) (CPU: %ws) (User: %ws) %ws",
			pi->ProcessId, pi->SessionId, pi->NumberOfThreads, pi->HandleCount,
			(PCWSTR)GetCpuTime(pi),
			(PCWSTR)GetUserNameFromSid(pi->pUserSid), pi->pProcessName);
	}
	::WTSFreeMemoryEx(WTSTypeProcessInfoLevel1, info, count);

	return true;
}

int main() {
	EnumerateProcesses1();
	EnumerateProcesses2();

	return 0;
}

