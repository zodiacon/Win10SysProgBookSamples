// thlist.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

struct ThreadInfo {
	DWORD Id;
	DWORD Pid;
	int Priority;
	FILETIME CreateTime;
	DWORD CPUTime;
	std::wstring ProcessName;
};

std::vector<ThreadInfo> EnumThreads(int pid);

int main(int argc, const char* argv[]) {
	DWORD pid = 0;
	if (argc > 1)
		pid = atoi(argv[1]);

	auto threads = EnumThreads(pid);
	printf("%6s %6s %5s %18s %11s %s\n", "TID", "PID", "Pri", "Started", "CPU Time", "Process Name");
	printf("%s\n", std::string(60, '-').c_str());

	for (auto& t : threads) {
		printf("%6d %6d %5d %18ws %11ws %ws\n", t.Id, t.Pid, t.Priority, 
			t.CreateTime.dwLowDateTime + t.CreateTime.dwHighDateTime == 0 ? L"(Unknown)" : (PCWSTR)CTime(t.CreateTime).Format(L"%x %X"), 
			(PCWSTR)CTimeSpan(t.CPUTime).Format(L"%D:%H:%M:%S"), t.ProcessName.c_str());
	}

	return 0;
}

std::vector<ThreadInfo> EnumThreads(int pid) {
	std::vector<ThreadInfo> threads;

	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS | TH32CS_SNAPTHREAD, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
		return threads;

	// enumerate processes first

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);

	std::unordered_map<DWORD, PROCESSENTRY32> processes;
	processes.reserve(512);

	::Process32First(hSnapshot, &pe);

	// skip idle process

	while (::Process32Next(hSnapshot, &pe)) {
		processes.insert({ pe.th32ProcessID, pe });
	}

	// enumerate threads

	threads.reserve(4096);

	THREADENTRY32 te;
	te.dwSize = sizeof(te);

	::Thread32First(hSnapshot, &te);

	do {
		if (te.th32OwnerProcessID > 0 && (pid == 0 || te.th32OwnerProcessID == pid)) {
			ThreadInfo ti;
			ti.Id = te.th32ThreadID;
			ti.Pid = te.th32OwnerProcessID;
			ti.Priority = te.tpBasePri;
			ti.ProcessName = processes[ti.Pid].szExeFile;
			auto hThread = ::OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, ti.Id);
			if (hThread) {
				FILETIME user, kernel, exit;
				::GetThreadTimes(hThread, &ti.CreateTime, &exit, &kernel, &user);
				ti.CPUTime = DWORD((*(ULONGLONG*)&kernel + *(ULONGLONG*)&user) / 10000000);
				::CloseHandle(hThread);
			}
			else {
				ti.CPUTime = 0;
				ti.CreateTime.dwHighDateTime = ti.CreateTime.dwLowDateTime = 0;
			}
			threads.push_back(std::move(ti));
		}
	} while (::Thread32Next(hSnapshot, &te));

	::CloseHandle(hSnapshot);

	return threads;
}
