// runas.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <conio.h>

int Error(const char* msg) {
	printf("%s (%u)\n", msg, ::GetLastError());
	return 1;
}

int GetPassword(WCHAR* password, int maxLen) {
	int count = 0;
	while (count < maxLen - 1) {
		auto ch = ::_getwch();
		if (ch == L'\r')
			break;
		if (ch == 8) {
			if (count == 0)
				continue;
			count--;
			printf("\b \b");
			continue;
		}
		printf("*");
		password[count++] = ch;
	}
	password[count] = 0;
	printf("\n");
	return count;
}

int wmain(int argc, wchar_t* argv[]) {
	if (argc < 3) {
		printf("Usage: runas <[domain\\]username> <\"commandline\">\n");
		return 0;
	}

	printf("Password: ");
	WCHAR password[64];
	GetPassword(password, _countof(password));

	// check for domain
	PCWSTR domain = L".";
	PCWSTR username = argv[1];
	if (::wcschr(argv[1], L'@'))
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

