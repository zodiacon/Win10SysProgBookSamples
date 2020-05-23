// setvirt.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>

int Error(const char* msg) {
	printf("%s (%u)\n", msg, ::GetLastError());
	return 1;
}

int main(int argc, const char* argv[]) {
	if (argc < 3) {
		printf("Usage: setvirt <pid> <on|off>\n");
		return 0;
	}

	HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, atoi(argv[1]));
	if (!hProcess)
		return Error("Failed to open process");

	HANDLE hToken;
	if (!::OpenProcessToken(hProcess, TOKEN_ADJUST_DEFAULT, &hToken))
		return Error("Failed to open token");

	ULONG set = ::_stricmp(argv[2], "on") == 0 ? 1 : 0;
	if (!::SetTokenInformation(hToken, TokenVirtualizationEnabled, &set, sizeof(set)))
		return Error("Failed to set token information");

	printf("Operation successful.\n");

	::CloseHandle(hToken);
	::CloseHandle(hProcess);

	return 0;
}

