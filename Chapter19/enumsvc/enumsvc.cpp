// enumsvc.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <string>
#include <assert.h>
#include <memory>

int DisplayServiceStatus(const wchar_t* name);
int Error(const char* msg);
int GetServiceStatus(const wchar_t* name, SERVICE_STATUS_PROCESS& status);
std::wstring ServiceTypeToString(DWORD type);
PCWSTR ErrorControlToString(DWORD ec);
PCWSTR ServiceStateToString(DWORD state);
PCWSTR ServiceStateToString(DWORD state);
std::wstring ServiceControlsAcceptedToString(DWORD accepted);

int wmain(int argc, const wchar_t* argv[]) {
	if(argc > 1)
		return DisplayServiceStatus(argv[1]);
	
	auto hScm = ::OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
	if (!hScm)
		return Error("Failed to open handle to SCM");

	auto size = 256 << 10;
	auto buffer = std::make_unique<BYTE[]>(size);
	DWORD returnedSize;
	DWORD count;
	if (!::EnumServicesStatusEx(hScm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
		buffer.get(), size, &returnedSize, &count, nullptr, nullptr))
		return Error("Failed in enumeration");

	int len = printf("%-30s %-35s %-18s %-9s %6s %s\n", "Name", "Display Name", "Type", "State", "PID", "Accepted");
	printf("%s\n", std::string(len, '-').c_str());
	auto status = (ENUM_SERVICE_STATUS_PROCESS*)buffer.get();

	for (DWORD i = 0; i < count; i++) {
		auto& s = status[i];
		printf("%-30ws %-30ws %-18ws %-9ws %6d %ws\n",
			std::wstring(s.lpServiceName).substr(0, 30).c_str(),
			std::wstring(s.lpDisplayName).substr(0, 35).c_str(),
			ServiceTypeToString(s.ServiceStatusProcess.dwServiceType).c_str(),
			ServiceStateToString(s.ServiceStatusProcess.dwCurrentState),
			s.ServiceStatusProcess.dwProcessId,
			ServiceControlsAcceptedToString(s.ServiceStatusProcess.dwControlsAccepted).c_str());

	}

	::CloseServiceHandle(hScm);

	return 0;
}

int Error(const char* msg) {
	printf("%s (%u)\n", msg, ::GetLastError());
	return 1;
}

int GetServiceStatus(const wchar_t* name, SERVICE_STATUS_PROCESS& status) {
	auto hScm = ::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
	if (!hScm)
		return Error("Failed to open handle to SCM");

	auto hService = ::OpenService(hScm, name, SERVICE_QUERY_STATUS);
	if (!hService)
		return Error("Failed to open handle to service");

	DWORD size = sizeof(status);
	if (!::QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (PBYTE)&status, sizeof(status), &size))
		return Error("Failed to query service status");

	::CloseServiceHandle(hService);
	::CloseServiceHandle(hScm);

	return 0;
}

int DisplayServiceStatus(const wchar_t* name) {
	SERVICE_STATUS_PROCESS status;
	if (GetServiceStatus(name, status))
		return 1;

	printf("Service type: %ws\n", ServiceTypeToString(status.dwServiceType).c_str());
	printf("Service state: %ws\n", ServiceStateToString(status.dwCurrentState));
	if (status.dwCurrentState != SERVICE_STOPPED) {
		printf("Process ID: %u\n", status.dwProcessId);
	}
	printf("Controls accepted: %ws\n", ServiceControlsAcceptedToString(status.dwControlsAccepted).c_str());

	return 0;
}

std::wstring ServiceTypeToString(DWORD type) {
	static struct {
		DWORD type;
		PCWSTR text;
	} types[] = {
		{ SERVICE_KERNEL_DRIVER, L"Kernel Driver" },
		{ SERVICE_FILE_SYSTEM_DRIVER, L"FS/Filter Driver" },
		{ SERVICE_WIN32_OWN_PROCESS, L"Own" },
		{ SERVICE_WIN32_SHARE_PROCESS, L"Shared" },
		{ SERVICE_INTERACTIVE_PROCESS, L"Interactive" },
		{ SERVICE_USER_SERVICE, L"User" },
		{ SERVICE_USERSERVICE_INSTANCE, L"Instance" },
	};

	std::wstring text;
	for (auto& item : types)
		if ((item.type & type) == item.type)
			text += std::wstring(item.text) + L", ";

	return text.substr(0, text.size() - 2);
}

PCWSTR ErrorControlToString(DWORD ec) {
	switch (ec) {
		case SERVICE_ERROR_NORMAL: return L"Normal (1)";
		case SERVICE_ERROR_IGNORE: return L"Ignore (0)";
		case SERVICE_ERROR_CRITICAL: return L"Critical (3)";
		case SERVICE_ERROR_SEVERE: return L"Severe (2)";
	}
	assert(false);
	return L"";
}

PCWSTR ServiceStateToString(DWORD state) {
	switch (state) {
		case SERVICE_RUNNING: return L"Running";
		case SERVICE_STOPPED: return L"Stopped";
		case SERVICE_PAUSED: return L"Paused";
		case SERVICE_START_PENDING: return L"Start Pending";
		case SERVICE_CONTINUE_PENDING: return L"Continue Pending";
		case SERVICE_STOP_PENDING: return L"Stop Pending";
		case SERVICE_PAUSE_PENDING: return L"Pause Pending";
	}
	return L"Unknown";
}

std::wstring ServiceControlsAcceptedToString(DWORD accepted) {
	static struct {
		DWORD type;
		PCWSTR text;
	} types[] = {
		{ SERVICE_ACCEPT_STOP, L"Stop" },
		{ SERVICE_ACCEPT_PAUSE_CONTINUE, L"Pause, Continue" },
		{ SERVICE_ACCEPT_SHUTDOWN, L"Shutdown" },
		{ SERVICE_ACCEPT_PARAMCHANGE, L"Param Change" },
		{ SERVICE_ACCEPT_NETBINDCHANGE, L"NET Bind Change" },
		{ SERVICE_ACCEPT_HARDWAREPROFILECHANGE, L"Hardware Profile Change" },
		{ SERVICE_ACCEPT_POWEREVENT, L"Power Event" },
		{ SERVICE_ACCEPT_SESSIONCHANGE, L"Session Change" },
		{ SERVICE_ACCEPT_PRESHUTDOWN, L"Pre Shutdown" },
		{ SERVICE_ACCEPT_TIMECHANGE, L"Time Change" },
		{ SERVICE_ACCEPT_TRIGGEREVENT, L"Trigger Event" },
		{ SERVICE_ACCEPT_USER_LOGOFF, L"User Log off" },
	};

	std::wstring text;
	for (auto& item : types)
		if ((item.type & accepted) == item.type)
			text += std::wstring(item.text) + L", ";

	return text.substr(0, text.size() - 2);
}
