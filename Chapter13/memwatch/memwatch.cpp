// memwatch.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <stdio.h>
#include <Psapi.h>

int Error(const char* msg) {
	printf("%s (%u)\n", msg, ::GetLastError());
	return 1;
}

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		printf("Usage: memwatch <pid>\n");
		return 0;
	}

	HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS,// PROCESS_QUERY_INFORMATION | SYNCHRONIZE, 
		FALSE, atoi(argv[1]));
	if (!hProcess)
		return Error("Failed to open process");

	if (!::InitializeProcessForWsWatch(hProcess))
		return Error("Failed to init WS watch");

	PSAPI_WS_WATCH_INFORMATION_EX info[1024];
	DWORD size = sizeof(info);

	while (::WaitForSingleObject(hProcess, 0) == WAIT_TIMEOUT) {
		if (!::GetWsChangesEx(hProcess, info, &size)) {
			printf("Insufficient buffer... too many changes\n");
			break;
		}
		for (int i = 0; i < _countof(info); i++) {
			auto& p = info[i];
			if (p.BasicInfo.FaultingPc == nullptr)
				break;

			printf("TID: %5u, VA: 0x%p, PA: 0x%p\n", (ULONG)p.FaultingThreadId, 
				p.BasicInfo.FaultingVa, p.BasicInfo.FaultingPc);
		}
	}
	::CloseHandle(hProcess);
}

