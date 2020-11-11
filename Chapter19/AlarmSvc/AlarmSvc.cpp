// AlarmSvc.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "Common.h"

void WINAPI AlarmMain(DWORD dwNumServicesArgs, LPTSTR* lpServiceArgVectors);
DWORD WINAPI AlarmHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
void SetStatus(DWORD status);
void HandleMessage(const AlarmMessage& msg);
void NTAPI OnTimerExpired(
	_Inout_     PTP_CALLBACK_INSTANCE Instance,
	_Inout_opt_ PVOID                 Context,
	_Inout_     PTP_TIMER             Timer);

#pragma comment(lib, "wtsapi32")

SERVICE_STATUS_HANDLE g_hService;
SERVICE_STATUS g_Status;
HANDLE g_hStopEvent;
HANDLE g_hMailslot;
PTP_TIMER g_Timer;

int main() {
	WCHAR name[] = L"alarm";
	const SERVICE_TABLE_ENTRY table[] = {
		{ name, AlarmMain },
		{ nullptr, nullptr }
	};
	if (!::StartServiceCtrlDispatcher(table))
		return 1;

	return 0;
}

void WINAPI AlarmMain(DWORD dwNumServicesArgs, LPTSTR* lpServiceArgVectors) {
	g_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_Status.dwWaitHint = 500;

	bool error = true;
	do {
		g_hStopEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		g_hService = ::RegisterServiceCtrlHandlerEx(L"alarmsvc", AlarmHandler, nullptr);
		if (!g_hService || !g_hStopEvent)
			break;

		SetStatus(SERVICE_START_PENDING);

		BYTE worldSid[SECURITY_MAX_SID_SIZE], systemSid[SECURITY_MAX_SID_SIZE];
		DWORD len = sizeof(worldSid);
		auto pWorldSid = (PSID)worldSid;
		auto pSystemSid = (PSID)systemSid;

		if (!::CreateWellKnownSid(WinWorldSid, nullptr, pWorldSid, &len))
			break;

		len = sizeof(systemSid);
		if (!::CreateWellKnownSid(WinLocalSystemSid, nullptr, pSystemSid, &len))
			break;

		PSECURITY_DESCRIPTOR sd = ::HeapAlloc(::GetProcessHeap(), 0, SECURITY_DESCRIPTOR_MIN_LENGTH);
		if (!sd)
			break;
		if (!::InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION))
			break;

		EXPLICIT_ACCESS ea[2] = { 0 };
		ea[0].grfAccessPermissions = FILE_ALL_ACCESS;
		ea[0].grfAccessMode = SET_ACCESS;
		ea[0].grfInheritance = NO_INHERITANCE;
		ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea[0].Trustee.ptstrName = (PWSTR)pSystemSid;

		ea[1].grfAccessPermissions = FILE_GENERIC_WRITE | FILE_GENERIC_READ;
		ea[1].grfAccessMode = SET_ACCESS;
		ea[1].grfInheritance = NO_INHERITANCE;
		ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea[1].Trustee.ptstrName = (PWSTR)pWorldSid;
		PACL dacl;
		if (ERROR_SUCCESS != ::SetEntriesInAcl(_countof(ea), ea, nullptr, &dacl))
			break;

		if (!::SetSecurityDescriptorOwner(sd, pSystemSid, FALSE))
			break;

		if (!::SetSecurityDescriptorDacl(sd, TRUE, dacl, FALSE))
			break;

		SECURITY_ATTRIBUTES sa = { sizeof(sa) };
		sa.lpSecurityDescriptor = sd;
		g_hMailslot = ::CreateMailslot(L"\\\\.\\mailslot\\Alarm", 1024, MAILSLOT_WAIT_FOREVER, &sa);
		if (g_hMailslot == INVALID_HANDLE_VALUE)
			break;

		::HeapFree(::GetProcessHeap(), 0, sd);
		::LocalFree(dacl);
		error = false;
	} while (false);

	if (error) {
		SetStatus(SERVICE_STOPPED);
		return;
	}

	SetStatus(SERVICE_RUNNING);

	DWORD msgSize, count;
	AlarmMessage msg;

	while (::WaitForSingleObject(g_hStopEvent, 1000) == WAIT_TIMEOUT) {
		do {
			if (!::GetMailslotInfo(g_hMailslot, nullptr, &msgSize, &count, nullptr))
				break;
			if (msgSize == MAILSLOT_NO_MESSAGE)
				break;

			DWORD bytes;
			if (msgSize == sizeof(msg) && ::ReadFile(g_hMailslot, &msg, sizeof(msg), &bytes, nullptr)) {
				HandleMessage(msg);
			}
			count--;
		} while (count > 0);

	}
	SetStatus(SERVICE_STOPPED);

	::CloseHandle(g_hStopEvent);
	::CloseHandle(g_hMailslot);

}

void HandleMessage(const AlarmMessage& msg) {
	switch (msg.Type) {
		case MessageType::AddAlarm:
			if (g_Timer == nullptr)
				g_Timer = ::CreateThreadpoolTimer(OnTimerExpired, nullptr, nullptr);
			::SetThreadpoolTimer(g_Timer, (PFILETIME)&msg.Time, 0, 1000);
			break;

		case MessageType::RemoveAlarm:
			if (g_Timer) {
				::CloseThreadpoolTimer(g_Timer);
				g_Timer = nullptr;
			}
			break;
	}
}

DWORD WINAPI AlarmHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) {
	switch (dwControl) {
		case SERVICE_CONTROL_STOP:
			SetStatus(SERVICE_STOP_PENDING);
			::SetEvent(g_hStopEvent);
			break;
	}
	return 0;
}

void SetStatus(DWORD status) {
	g_Status.dwCurrentState = status;
	g_Status.dwControlsAccepted = status == SERVICE_RUNNING ? SERVICE_CONTROL_STOP : 0;
	::SetServiceStatus(g_hService, &g_Status);
}

void NTAPI OnTimerExpired(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_TIMER Timer) {
	WCHAR title[] = L"!!! ALARM !!!";
	WCHAR msg[] = L"It's time to wake up!";
	DWORD response;

	::WTSSendMessage(nullptr, ::WTSGetActiveConsoleSessionId(),
		title, sizeof(title), msg, sizeof(msg),
		MB_OK, 0, &response, FALSE);
}

