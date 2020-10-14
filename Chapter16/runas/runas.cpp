// runas.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <conio.h>

int Error(const char* msg) {
	printf("%s (%u)\n", msg, ::GetLastError());
	return 1;
}

int wmain(int argc, wchar_t* argv[]) {
	if (argc < 3) {
		printf("Usage: runas <[domain\\]username> <\"commandline\">\n");
		return 0;
	}

	printf("Password: ");
	WCHAR password[64];
	int count = 0;
	while (count < _countof(password) - 1) {
		auto ch = ::_getwch();
		if (ch == L'\r')
			break;
		printf("*");
		password[count++] = ch;
	}
	password[count] = 0;
	printf("\n");

	// check for domain
	PCWSTR domain = L".";
	PCWSTR username = argv[1];
	if (::wcschr(argv[1], '@'))
		domain = nullptr;
	else {
		auto backslash = ::wcschr(argv[1], L'\\');
		if (backslash) {
			domain = argv[1];
			*backslash = L'\0';
			username = backslash + 1;
		}
	}

	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	BOOL success = ::CreateProcessWithLogonW(username, domain, password, 
		LOGON_WITH_PROFILE, nullptr, argv[2],
		0, nullptr, nullptr, &si, &pi);
	if (!success)
		return Error("Error in CreateProcessWithLogonW");

	::SecureZeroMemory(password, sizeof(password));

	printf("Process %u created successfully!\n", pi.dwProcessId);

	::CloseHandle(pi.hProcess);
	::CloseHandle(pi.hThread);

	return 0;
}

