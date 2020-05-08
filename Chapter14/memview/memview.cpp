// memview.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>

int main() {
	HANDLE hMemMap = ::OpenFileMapping(FILE_MAP_READ, FALSE, L"MySharedMemory");
	if (!hMemMap) {
		printf("File mapping object is not available\n");
		return 1;
	}

	WCHAR text[1024] = { 0 };
	auto data = (const WCHAR*)::MapViewOfFile(hMemMap, FILE_MAP_READ, 0, 0, 0);
	if (!data) {
		printf("Failed to map shared memory\n");
		return 1;
	}

	for (;;) {
		if (::_wcsicmp(text, data) != 0) {
			// text changed, update and display
			::wcscpy_s(text, data);
			printf("%ws\n", text);
		}
		::Sleep(1000);
	}

	::UnmapViewOfFile(data);
	::CloseHandle(hMemMap);
	return 0;
}
