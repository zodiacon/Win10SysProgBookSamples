// JobTree.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <string>

HANDLE CreateSimpleProcess(PCWSTR name) {
	std::wstring sname(name);
	PROCESS_INFORMATION pi;
	STARTUPINFO si = { sizeof(si) };
	if (!::CreateProcess(nullptr, const_cast<PWSTR>(sname.data()), nullptr, nullptr, FALSE, 
		CREATE_BREAKAWAY_FROM_JOB | CREATE_NEW_CONSOLE,
		nullptr, nullptr, &si, &pi))
		return nullptr;

	::CloseHandle(pi.hThread);
	return pi.hProcess;
}

HANDLE CreateJobHierarchy() {
	auto hJob1 = ::CreateJobObject(nullptr, L"Job1");
	assert(hJob1);
	auto hProcess1 = CreateSimpleProcess(L"mspaint");

	auto success = ::AssignProcessToJobObject(hJob1, hProcess1);
	assert(success);

	auto hJob2 = ::CreateJobObject(nullptr, L"Job2");
	assert(hJob2);
	success = ::AssignProcessToJobObject(hJob2, hProcess1);
	assert(success);

	auto hProcess2 = CreateSimpleProcess(L"mstsc");
	success = ::AssignProcessToJobObject(hJob2, hProcess2);
	assert(success);

	auto hProcess3 = CreateSimpleProcess(L"cmd");
	success = ::AssignProcessToJobObject(hJob1, hProcess3);
	assert(success);

	// not bothering closing process and job2 handles

	return hJob1;
}

int main() {
	auto hJob = CreateJobHierarchy();
	printf("Press any key to terminate parent job...\n");
	::getchar();
	::TerminateJobObject(hJob, 0);

	return 0;
}

