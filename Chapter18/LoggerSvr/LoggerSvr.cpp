// LoggerSvr.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <assert.h>

int Error(const char* msg, DWORD error = ::GetLastError()) {
	printf("%s (%u)\n", msg, error);
	return 1;
}

int main() {
	HANDLE hMailSlot = ::CreateMailslot(L"\\\\.\\mailslot\\loggerbox", 1024, MAILSLOT_WAIT_FOREVER, nullptr);
	if (hMailSlot == INVALID_HANDLE_VALUE)
		return Error("Failed to create mailslot");

	printf("Mailslot created, listening...\n");

	DWORD nextSize;
	char message[1024];
	DWORD read;
	SYSTEMTIME st;

	while (::GetMailslotInfo(hMailSlot, nullptr, &nextSize, nullptr, nullptr)) {
		if (nextSize == MAILSLOT_NO_MESSAGE) {
			::Sleep(100);
			continue;
		}
		if (!::ReadFile(hMailSlot, message, nextSize, &read, nullptr))
			continue;

		::GetLocalTime(&st);
		printf("%02d:%02d:%02d.%03d: %s\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, message);
	}

	::CloseHandle(hMailSlot);

	return 0;
}

