// SimplePipe.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>

int Error(const char* msg, DWORD error = ::GetLastError()) {
	printf("%s (%u)\n", msg, error);
	return 1;
}

int main() {
	HANDLE hRead, hWrite;
	if (!::CreatePipe(&hRead, &hWrite, nullptr, 0))
		return Error("Failed to create pipe");

	//
	// make the write handle inheritable
	//
	::SetHandleInformation(hWrite, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

	PROCESS_INFORMATION pi;
	STARTUPINFO si = { sizeof(si) };
	si.hStdOutput = hWrite;
	si.dwFlags = STARTF_USESTDHANDLES;

	WCHAR name[] = L"cmd.exe";
	if (!::CreateProcess(nullptr, name, nullptr, nullptr, TRUE, 
		CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi))
		return Error("Failed to create cmd process");

	::CloseHandle(pi.hThread); 	// thread handle not needed

	//
	// we don't need the write handle anymore
	//
	::CloseHandle(hWrite);

	char text[512];
	DWORD read;
	while (::WaitForSingleObject(pi.hProcess, 0) == WAIT_TIMEOUT) {
		if (!::ReadFile(hRead, text, sizeof(text) - 1, &read, nullptr))
			break;

		// NULL-terminate the string
		text[read] = 0;
		printf("%s", text);
	}

	::CloseHandle(hRead);
	::CloseHandle(pi.hProcess);

	return 0;
}

