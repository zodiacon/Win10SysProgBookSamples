// MutexDemo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

bool WriteToFile(PCWSTR path);

int Error(const char* text) {
	printf("%s (%d)\n", text, ::GetLastError());
	return 1;
}

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2) {
		printf("Usage: MutexDemo <file>\n");
		return 0;
	}

	HANDLE hMutex = ::CreateMutex(nullptr, FALSE, L"ExampleMutex");
	if (!hMutex)
		return Error("Failed to create/open mutex");

	printf("Process %d. Mutex handle: 0x%X\n", ::GetCurrentProcessId(), HandleToULong(hMutex));
	printf("Press any key to begin...\n");
	_getch();
	printf("Working...\n");

	for (int i = 0; i < 100; i++) {
		// insert some randomness
		::Sleep(::GetTickCount() & 0xff);

		// acquire the mutex
		::WaitForSingleObject(hMutex, INFINITE);

		// write to the file
		if (!WriteToFile(argv[1]))
			return Error("Failed to write to file");

		::ReleaseMutex(hMutex);
	}

	::CloseHandle(hMutex);
	printf("Done.\n");

	return 0;
}

bool WriteToFile(PCWSTR path) {
	HANDLE hFile = ::CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	::SetFilePointer(hFile, 0, nullptr, FILE_END);
	char text[128];
	sprintf_s(text, "This is text from process %d\n", ::GetCurrentProcessId());
	DWORD bytes;
	BOOL ok = ::WriteFile(hFile, text, (DWORD)strlen(text), &bytes, nullptr);
	::CloseHandle(hFile);

	return ok;
}

