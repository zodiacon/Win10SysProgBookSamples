// TestLog.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>

int Error(const char* msg, DWORD error = ::GetLastError()) {
	printf("%s (%u)\n", msg, error);
	return 1;
}

int main() {
	HANDLE hFile = ::CreateFile(L"\\\\.\\mailslot\\loggerbox", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return Error("Failed to open mailslot");

	DWORD written;
	char text[256];
	do {
		printf("Enter message (q to quit): ");
		gets_s(text);
		if (_stricmp(text, "q") == 0)
			break;
	} while (::WriteFile(hFile, text, (DWORD)strlen(text) + 1, &written, nullptr));

	::CloseHandle(hFile);
	return 0;
}

