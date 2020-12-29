// GenSvc.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "FileLogger.h"

SERVICE_STATUS_HANDLE g_hService;
SERVICE_STATUS g_Status;
HANDLE g_hStopEvent;
std::unique_ptr<FileLogger> s_Logger;

void SetStatus(DWORD status) {
	g_Status.dwCurrentState = status;
	g_Status.dwControlsAccepted = SERVICE_ACCEPT_STOP |
		SERVICE_ACCEPT_HARDWAREPROFILECHANGE | SERVICE_ACCEPT_NETBINDCHANGE | SERVICE_ACCEPT_PARAMCHANGE |
		SERVICE_ACCEPT_PAUSE_CONTINUE | SERVICE_ACCEPT_POWEREVENT | SERVICE_ACCEPT_PRESHUTDOWN |
		SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_SESSIONCHANGE;
	::SetServiceStatus(g_hService, &g_Status);
}

DWORD WINAPI ServiceHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) {
	s_Logger->Debug(L"ServiceHandler: Control: %u Type: %u", dwControl, dwEventType);

	switch (dwControl) {
		case SERVICE_CONTROL_STOP:
			SetStatus(SERVICE_STOP_PENDING);
			::SetEvent(g_hStopEvent);
			break;

		case SERVICE_CONTROL_PAUSE:
			SetStatus(SERVICE_PAUSED);
			break;

		case SERVICE_CONTROL_CONTINUE:
			SetStatus(SERVICE_RUNNING);
			break;
	}
	return 0;
}

void WINAPI ServiceMain(DWORD dwNumServicesArgs, LPTSTR* lpServiceArgVectors) {
	g_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_Status.dwWaitHint = 500;

	bool error = true;
	do {
		g_hService = ::RegisterServiceCtrlHandlerEx(L"gensvc", ServiceHandler, nullptr);
		if (!g_hService)
			break;

		SetStatus(SERVICE_START_PENDING);

		g_hStopEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (!g_hStopEvent)
			break;

		PWSTR path;
		if (FAILED(::SHGetKnownFolderPath(FOLDERID_PublicDocuments, 0, nullptr, &path)))
			break;

		std::wstring fullpath(path);
		fullpath += L"\\gensvc.log";
		::CoTaskMemFree(path);
		s_Logger = std::make_unique<FileLogger>(fullpath.c_str());
		if (!s_Logger)
			break;

		s_Logger->Info(L"Logger initialized, file: %ws", fullpath.c_str());

		error = false;
	} while (false);

	if (error) {
		SetStatus(SERVICE_STOPPED);
		return;
	}

	SetStatus(SERVICE_RUNNING);

	::WaitForSingleObject(g_hStopEvent, INFINITE);
	s_Logger->Info(L"Stop event signaled");
	
	SetStatus(SERVICE_STOPPED);
	s_Logger->Info(L"Service stopped");
}

int main() {
	WCHAR name[] = L"gensvc";
	const SERVICE_TABLE_ENTRY table[] = {
		{ name, ServiceMain },
		{ nullptr, nullptr }
	};
	if (!::StartServiceCtrlDispatcher(table))
		return 1;

	s_Logger.reset();

	return 0;
}

