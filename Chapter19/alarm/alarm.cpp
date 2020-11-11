// alarm.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <string>
#include "..\AlarmSvc\Common.h"

int Error(const char* msg) {
	printf("%s (%u)\n", msg, ::GetLastError());
	return 1;
}

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		printf("Usage: alarm <set hh:mm:ss | remove>\n");
		return 0;
	}

	HANDLE hFile = ::CreateFile(L"\\\\.\\mailslot\\Alarm", GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return Error("Failed to open mailslot");

	DWORD bytes;

	if (_stricmp(argv[1], "set") == 0 && argc > 2) {
		AlarmMessage msg;
		msg.Type = MessageType::AddAlarm;
		std::string time(argv[2]);
		if (time.size() != 8) {
			printf("Time format is illegal.\n");
			return 1;
		}
		SYSTEMTIME st;
		::GetSystemTime(&st);
		st.wHour = atoi(time.substr(0, 2).c_str());
		st.wMinute = atoi(time.substr(3, 2).c_str());
		st.wSecond = atoi(time.substr(6, 2).c_str());
		::SystemTimeToFileTime(&st, &msg.Time);
		::LocalFileTimeToFileTime(&msg.Time, &msg.Time);
		if (!::WriteFile(hFile, &msg, sizeof(msg), &bytes, nullptr))
			return Error("failed in WriteFile");
	}
	else if (_stricmp(argv[1], "remove") == 0) {
		AlarmMessage msg;
		msg.Type = MessageType::RemoveAlarm;
		if (!::WriteFile(hFile, &msg, sizeof(msg), &bytes, nullptr))
			return Error("failed in WriteFile");
	}
	else {
		printf("Error in command line arguments.\n");
	}
	::CloseHandle(hFile);

	return 0;
}
