// svcpriv.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>

int Error(const char* msg) {
	printf("%s (%u)\n", msg, ::GetLastError());
	return 1;
}

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 3) {
		printf("Usage: svcpriv <servicename> <privilege1> [...]\n");
		return 0;
	}

	auto hScm = ::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
	if (!hScm)
		return Error("Failed to open handle to SCM");

	auto hService = ::OpenService(hScm, argv[1], SERVICE_CHANGE_CONFIG);
	if (!hService)
		return Error("Failed to open service handle");

	SERVICE_REQUIRED_PRIVILEGES_INFO info;
	WCHAR buffer[1024];
	size_t index = 0;
	for (int i = 2; i < argc; i++) {
		::wcscpy_s(buffer + index, _countof(buffer) - index, L"Se");
		::wcscat_s(buffer + index, _countof(buffer) - index, argv[i]);
		::wcscat_s(buffer + index, _countof(buffer) - index, L"Privilege");
		index += ::wcslen(argv[i]) + 1 + ::wcslen(L"SePrivilege");
	}
	buffer[index] = 0;
	info.pmszRequiredPrivileges = buffer;
	if(!::ChangeServiceConfig2(hService, SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO, &info))
		return Error("Failed to change config");

	printf("Configuration changed successfully.\n");

	::CloseServiceHandle(hScm);
	::CloseServiceHandle(hService);

	return 0;
}

