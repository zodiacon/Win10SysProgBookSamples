// snapproc.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <processsnapshot.h>
#include <stdio.h>
#include <string>

HPSS CreateSnapshot(DWORD pid, PSS_CAPTURE_FLAGS flags, DWORD& error);
void DisplayProcessInfo(HPSS hSnapshot);
void DisplayHandlesInfo(HPSS hSnapshot);
void DisplayThreadInfo(HPSS hSnapshot);
std::string TimeToString(const FILETIME& ft);
std::string TimeSpanToString(const FILETIME& ft);

int main(int argc, const char* argv[]) {
	if(argc < 2) {
		printf("Usage: snapproc <pid> [htvm]\n");
		return 0;
	}

	DWORD pid = atoi(argv[1]);
	DWORD error;
	PSS_CAPTURE_FLAGS flags = PSS_CAPTURE_NONE;
	if(argc > 2) {
		char ch;
		const char* s = argv[2];
		while(ch = *s++) {
			switch(ch) {
				case 'h': case 'H':
					flags |= PSS_CAPTURE_HANDLES | PSS_CAPTURE_HANDLE_NAME_INFORMATION | PSS_CAPTURE_HANDLE_BASIC_INFORMATION;
					break;

				case 't': case 'T':
					flags |= PSS_CAPTURE_THREADS;
					break;

				case 'v': case 'V':
					flags |= PSS_CAPTURE_VA_SPACE;
					break;

				case 'm': case 'M':
					flags |= PSS_CAPTURE_VA_CLONE;
					break;

			}
		}
	}

	auto hSnapshot = CreateSnapshot(pid, flags, error);
	if(hSnapshot == nullptr) {
		printf("Error capturing snapshot: %u\n", error);
		return 1;
	}

	DisplayProcessInfo(hSnapshot);
	if(flags & PSS_CAPTURE_HANDLES)
		DisplayHandlesInfo(hSnapshot);

	if(flags & PSS_CAPTURE_THREADS)
		DisplayThreadInfo(hSnapshot);

	::PssFreeSnapshot(::GetCurrentProcess(), hSnapshot);

	return 0;
}

std::string TimeToString(const FILETIME& ft) {
	SYSTEMTIME st;
	::FileTimeToSystemTime(&ft, &st);
	char text[128];
	::sprintf_s(text, "%02d/%02d %02d:%02d:%02d.%03d", 
		st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	return text;
}

std::string TimeSpanToString(const FILETIME& ft) {
	char text[32];
	// time is given in 100 nsec units
	auto value = *(DWORD64*)&ft;
	auto msec = value / 10000;
	auto usec = (value - msec * 10000) / 10;
	::sprintf_s(text, "%lld.%03lld msec", msec, usec);
	return text;
}

HPSS CreateSnapshot(DWORD pid, PSS_CAPTURE_FLAGS flags, DWORD& error) {
	HANDLE hProcess = ::OpenProcess(MAXIMUM_ALLOWED, FALSE, pid);
	if(!hProcess) {
		error = ::GetLastError();
		return nullptr;
	}

	HPSS hSnapShot;
	error = ::PssCaptureSnapshot(hProcess, flags, 0, &hSnapShot);
	::CloseHandle(hProcess);

	if(ERROR_SUCCESS != error)
		return nullptr;

	return hSnapShot;
}

void DisplayProcessInfo(HPSS hSnapshot) {
	PSS_PROCESS_INFORMATION psinfo;
	if(ERROR_SUCCESS == ::PssQuerySnapshot(hSnapshot, PSS_QUERY_PROCESS_INFORMATION, &psinfo, sizeof(psinfo))) {
		printf("Image file: %ws\n", psinfo.ImageFileName);
		printf("PID: %u\n", psinfo.ProcessId);
		printf("Parent PID: %u\n", psinfo.ParentProcessId);
		printf("Create time: %s\n", TimeToString(psinfo.CreateTime).c_str());
		printf("User time: %s\n", TimeSpanToString(psinfo.UserTime).c_str());
		printf("Kernel time: %s\n", TimeSpanToString(psinfo.KernelTime).c_str());
		printf("Working set: %zd MB\n", psinfo.WorkingSetSize >> 20);
		printf("Commit size: %zd MB\n", psinfo.PagefileUsage >> 20);
		printf("Virtual size: %zd MB\n", psinfo.VirtualSize >> 20);
	}

}

void DisplayHandlesInfo(HPSS hSnapshot) {
	PSS_HANDLE_INFORMATION info;
	if(ERROR_SUCCESS != ::PssQuerySnapshot(hSnapshot, PSS_QUERY_HANDLE_INFORMATION, &info, sizeof(info))) {
		printf("No handle information\n");
		return;
	}

	printf("Handles captured: %u\n", info.HandlesCaptured);

	HPSSWALK hWalk;
	if(ERROR_SUCCESS == ::PssWalkMarkerCreate(nullptr, &hWalk)) {
		PSS_HANDLE_ENTRY handle;
		while(ERROR_SUCCESS == ::PssWalkSnapshot(hSnapshot, PSS_WALK_HANDLES, hWalk, &handle, sizeof(handle))) {
			printf("Handle: %4u  Name: %ws Type: %ws\n",
				HandleToULong(handle.Handle),
				std::wstring(handle.ObjectName, handle.ObjectNameLength / sizeof(WCHAR)).c_str(),
				std::wstring(handle.TypeName, handle.TypeNameLength / sizeof(WCHAR)).c_str());
		}
		::PssWalkMarkerFree(hWalk);
	}
}

void DisplayThreadInfo(HPSS hSnapshot) {
	PSS_THREAD_INFORMATION info;
	if(ERROR_SUCCESS != ::PssQuerySnapshot(hSnapshot, PSS_QUERY_THREAD_INFORMATION, &info, sizeof(info))) {
		printf("No thread information\n");
		return;
	}

	printf("Threads captured: %u\n", info.ThreadsCaptured);

	HPSSWALK hWalk;
	if(ERROR_SUCCESS == ::PssWalkMarkerCreate(nullptr, &hWalk)) {
		PSS_THREAD_ENTRY thread;
		while(ERROR_SUCCESS == ::PssWalkSnapshot(hSnapshot, PSS_WALK_THREADS, hWalk, &thread, sizeof(thread))) {
			printf("TID: %6u Created: %s Priority: %2d User: %s Kernel: %s\n",
				thread.ThreadId,
				TimeToString(thread.CreateTime).c_str(),
				thread.Priority,
				TimeSpanToString(thread.UserTime).c_str(), TimeSpanToString(thread.KernelTime).c_str());
		}
		::PssWalkMarkerFree(hWalk);
	}
}
