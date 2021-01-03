// DebugPrint.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <atltime.h>

int Error(const char* msg) {
	printf("%s (%u)\n", msg, ::GetLastError());
	return 1;
}

int main() {
	HANDLE hBufferReady = ::CreateEvent(nullptr, FALSE, FALSE, L"DBWIN_BUFFER_READY");
	if (!hBufferReady)
		return Error("Failed to create event");

	HANDLE hDataReady = ::CreateEvent(nullptr, FALSE, FALSE, L"DBWIN_DATA_READY");
	if (!hDataReady)
		return Error("Failed to create event");

	DWORD size = 1 << 12;
	HANDLE hMemFile = ::CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, size, L"DBWIN_BUFFER");
	if (!hMemFile)
		return Error("Failed to create MMF");

	auto buffer = (BYTE*)::MapViewOfFile(hMemFile, FILE_MAP_READ, 0, 0, 0);
	if (!buffer)
		return Error("Failed to map view");

	while(::SignalObjectAndWait(hBufferReady, hDataReady, INFINITE, FALSE) == WAIT_OBJECT_0) {
		SYSTEMTIME local;
		::GetLocalTime(&local);
		DWORD pid = *(DWORD*)buffer;
		printf("%ws.%03d %6d: %s\n", 
			(PCWSTR)CTime(local).Format(L"%X"), 
			local.wMilliseconds, pid, 
			(const char*)(buffer + sizeof(DWORD)));
	}

	// doesn't mean much...
	::UnmapViewOfFile(buffer);
	::CloseHandle(hMemFile);
	::CloseHandle(hDataReady);
	::CloseHandle(hBufferReady);

	return 0;
}

