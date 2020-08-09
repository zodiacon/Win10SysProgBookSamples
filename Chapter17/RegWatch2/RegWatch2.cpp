// RegWatch2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <atltime.h>
#include <atlstr.h>
#include "EventParser.h"
#include "TraceManager.h"

#pragma comment(lib, "tdh")

TraceManager* g_pMgr;
HANDLE g_hEvent;

void OnEvent(PEVENT_RECORD rec) {
	EventParser parser(rec);

	auto ts = parser.GetEventHeader().TimeStamp.QuadPart;
	printf("Time: %ws PID: %u: ", (PCWSTR)CTime(*(FILETIME*)&ts).Format(L"%c"), parser.GetProcessId());

	switch (parser.GetEventHeader().EventDescriptor.Opcode) {
		case EVENT_TRACE_TYPE_REGCREATE:				printf("Create key"); break;
		case EVENT_TRACE_TYPE_REGOPEN:					printf("Open key"); break;
		case EVENT_TRACE_TYPE_REGDELETE:				printf("Delete key"); break;
		case EVENT_TRACE_TYPE_REGQUERY:					printf("Query key"); break;
		case EVENT_TRACE_TYPE_REGSETVALUE:				printf("Set value"); break;
		case EVENT_TRACE_TYPE_REGDELETEVALUE:			printf("Delete value"); break;
		case EVENT_TRACE_TYPE_REGQUERYVALUE:			printf("Query value"); break;
		case EVENT_TRACE_TYPE_REGENUMERATEKEY:			printf("Enum key"); break;
		case EVENT_TRACE_TYPE_REGENUMERATEVALUEKEY:		printf("Enum values"); break;
		case EVENT_TRACE_TYPE_REGQUERYMULTIPLEVALUE:	printf("Query multiple values"); break;
		case EVENT_TRACE_TYPE_REGSETINFORMATION:		printf("Set key info"); break;
		case EVENT_TRACE_TYPE_REGCLOSE:					printf("Close key"); break;
		default:										printf("(Other)"); break;
	}

	auto prop = parser.GetProperty(L"KeyName");
	if (prop) {
		printf(" %ws", prop->GetUnicodeString());
	}
	printf("\n");
}

int main() {
	TraceManager mgr;
	if (!mgr.Start(OnEvent)) {
		printf("Failed to start trace. Are you running elevated?\n");
		return 1;
	}

	g_pMgr = &mgr;
	
	g_hEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);

	::SetConsoleCtrlHandler([](auto type) {
		if (type == CTRL_C_EVENT) {
			g_pMgr->Stop();
			::SetEvent(g_hEvent);
			return TRUE;
		}
		return FALSE;
		}, TRUE);

	::WaitForSingleObject(g_hEvent, INFINITE);
	::CloseHandle(g_hEvent);

	return 0;
}
