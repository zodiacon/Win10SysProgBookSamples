// ParseETW.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include <Windows.h>
#include <stdio.h>
#include <tdh.h>
#include <vector>
#include <assert.h>
#include <memory>
#include <atltime.h>
#include <in6addr.h>

#pragma comment(lib, "tdh")


void CALLBACK OnEvent(PEVENT_RECORD rec);
void DisplayEventInfo(PEVENT_RECORD rec, PTRACE_EVENT_INFO info);
void DisplayGeneralEventInfo(PEVENT_RECORD rec);

int wmain(int argc, wchar_t* argv[]) {
	if(argc < 2) {
		printf("Usage: ParseETW <file.etl>\n");
		return 0;
	}

	EVENT_TRACE_LOGFILE etl = { 0 };
	etl.LogFileName = argv[1];
	etl.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD;
	etl.EventRecordCallback = OnEvent;

	TRACEHANDLE hTrace = ::OpenTrace(&etl);
	if(hTrace == INVALID_PROCESSTRACE_HANDLE) {
		printf("Failed to open trace\n");
		return 1;
	}

	auto& header = etl.LogfileHeader;
	printf("OS Version: %d.%d Build: %d\n", 
		header.VersionDetail.MajorVersion, header.VersionDetail.MinorVersion, header.ProviderVersion);
	printf("Processors: %u\n", header.NumberOfProcessors);
	printf("Start: %ws\nEnd:   %ws\n", 
		(PCWSTR)CTime(*(FILETIME*)&header.StartTime).Format(L"%c"), 
		(PCWSTR)CTime(*(FILETIME*)&header.EndTime).Format(L"%c"));

	::ProcessTrace(&hTrace, 1, nullptr, nullptr);

	::CloseTrace(hTrace);

	return 0;
}

void CALLBACK OnEvent(PEVENT_RECORD rec) {
	DisplayGeneralEventInfo(rec);

	ULONG size = 0;
	auto status = ::TdhGetEventInformation(rec, 0, nullptr, nullptr, &size);
	assert(status == ERROR_INSUFFICIENT_BUFFER);

	auto buffer = std::make_unique<BYTE[]>(size);
	if(!buffer) {
		printf("Out of memory!\n");
		::ExitProcess(1);
	}

	auto info = reinterpret_cast<PTRACE_EVENT_INFO>(buffer.get());
	status = ::TdhGetEventInformation(rec, 0, nullptr, info, &size);
	if(status != ERROR_SUCCESS) {
		printf("Error processing event!\n");
		return;
	}

	DisplayEventInfo(rec, info);
}

void DisplayEventInfo(PEVENT_RECORD rec, PTRACE_EVENT_INFO info) {
	if(info->KeywordsNameOffset)
		printf("Keywords: %ws ", (PCWSTR)((BYTE*)info + info->KeywordsNameOffset));
	if(info->OpcodeNameOffset)
		printf("Opcode: %ws ", (PCWSTR)((BYTE*)info + info->OpcodeNameOffset));
	if(info->LevelNameOffset)
		printf("Level: %ws ", (PCWSTR)((BYTE*)info + info->LevelNameOffset));
	if(info->TaskNameOffset)
		printf("Task: %ws ", (PCWSTR)((BYTE*)info + info->TaskNameOffset));
	if(info->EventMessageOffset)
		printf("\nMessage: %ws", (PCWSTR)((BYTE*)info + info->EventMessageOffset));

	printf("\nProperties: %u\n", info->TopLevelPropertyCount);

	// properties data length and pointer
	auto userlen = rec->UserDataLength;
	auto data = (PBYTE)rec->UserData;

	auto pointerSize = (rec->EventHeader.Flags & EVENT_HEADER_FLAG_32_BIT_HEADER) ? 4 : 8;
	ULONG len;
	WCHAR value[512];

	for(DWORD i = 0; i < info->TopLevelPropertyCount; i++) {
		auto& pi = info->EventPropertyInfoArray[i];
		auto propName = (PCWSTR)((BYTE*)info + pi.NameOffset);
		printf(" Name: %ws ", propName);

		len = pi.length;
		if((pi.Flags & (PropertyStruct | PropertyParamCount)) == 0) {
			//
			// deal with simple properties only
			//
			PEVENT_MAP_INFO mapInfo = nullptr;
			std::unique_ptr<BYTE[]> mapBuffer;
			PWSTR mapName = nullptr;
			//
			// retrieve map information (if any)
			//
			if(pi.nonStructType.MapNameOffset) {
				ULONG size = 0;
				mapName = (PWSTR)((BYTE*)info + pi.nonStructType.MapNameOffset);
				if(ERROR_INSUFFICIENT_BUFFER == ::TdhGetEventMapInformation(rec, mapName, mapInfo, &size)) {
					mapBuffer = std::make_unique<BYTE[]>(size);
					mapInfo = reinterpret_cast<PEVENT_MAP_INFO>(mapBuffer.get());
					if(ERROR_SUCCESS != ::TdhGetEventMapInformation(rec, mapName, mapInfo, &size))
						mapInfo = nullptr;
				}
			}

			ULONG size = sizeof(value);
			USHORT consumed;
			// special case for IPv6 address
			if (pi.nonStructType.InType == TDH_INTYPE_BINARY && pi.nonStructType.OutType == TDH_OUTTYPE_IPV6)
				len = sizeof(IN6_ADDR);

			auto error = ::TdhFormatProperty(info, mapInfo, pointerSize,
				pi.nonStructType.InType, pi.nonStructType.OutType,
				(USHORT)len, userlen, data, &size, value, &consumed);
			if(ERROR_SUCCESS == error) {
				printf("Value: %ws", value);
				len = consumed;
				if(mapName)
					printf(" (%ws)", (PCWSTR)mapName);
				printf("\n");
			}
			else if(mapInfo) {
				error = ::TdhFormatProperty(info, nullptr, pointerSize,
					pi.nonStructType.InType, pi.nonStructType.OutType,
					(USHORT)len, userlen, data, &size, value, &consumed);
				if(ERROR_SUCCESS == error)
					printf("Value: %ws\n", value);
			}
			if(ERROR_SUCCESS != error)
				printf("(failed to get value)\n");
		}
		else {
			printf("(not a simple property)\n");
		}
		userlen -= (USHORT)len;
		data += len;
	}

	printf("\n");
}

void DisplayGeneralEventInfo(PEVENT_RECORD rec) {
	WCHAR sguid[64];
	auto& header = rec->EventHeader;
	::StringFromGUID2(header.ProviderId, sguid, _countof(sguid));

	printf("Provider: %ws Time: %ws PID: %u TID: %u\n",
		sguid, (PCWSTR)CTime(*(FILETIME*)&header.TimeStamp).Format(L"%c"),
		header.ProcessId, header.ThreadId);
}

