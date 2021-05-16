// RunETW.cpp : This file contains the 'main' function. Program execution begins and ends there.
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

bool RunSession(const std::vector<GUID>& providers, PCWSTR filename, bool realTime);
void CALLBACK OnEvent(PEVENT_RECORD rec);
void DisplayGeneralEventInfo(PEVENT_RECORD rec);
void DisplayEventInfo(PEVENT_RECORD rec, PTRACE_EVENT_INFO info);

HANDLE g_hStop;

std::vector<GUID> GetProviders(std::vector<PCWSTR> names) {
	std::vector<GUID> providers;
	providers.reserve(names.size());

	auto count = names.size();
	for(size_t i = 0; i < count; i++) {
		auto name = names[i];
		if(name[0] == L'{') {	// GUID rather than name
			GUID guid;
			if(S_OK == ::CLSIDFromString(name, &guid)) {
				providers.push_back(guid);
				names.erase(names.begin() + i);
				i--;
				count--;
			}
		}
	}

	if(names.empty())
		return providers;

	ULONG size = 0;
	auto error = ::TdhEnumerateProviders(nullptr, &size);
	assert(error == ERROR_INSUFFICIENT_BUFFER);

	// allocate with the required size
	auto buffer = std::make_unique<BYTE[]>(size);
	if(!buffer)
		return providers;

	auto data = reinterpret_cast<PROVIDER_ENUMERATION_INFO*>(buffer.get());
	// second call
	error = ::TdhEnumerateProviders(data, &size);
	assert(error == ERROR_SUCCESS);
	if(error != ERROR_SUCCESS)
		return providers;

	int found = 0;
	for(ULONG i = 0; i < data->NumberOfProviders && found < names.size(); i++) {
		const auto& item = data->TraceProviderInfoArray[i];
		auto name = (PCWSTR)(buffer.get() + item.ProviderNameOffset);
		for(auto n : names) {
			if(_wcsicmp(name, n) == 0) {
				providers.push_back(item.ProviderGuid);
				found++;
				break;
			}
		}
	}

	return providers;
}

int wmain(int argc, const wchar_t* argv[]) {
	if(argc < 2) {
		printf("Usage: RunETW2 [-o filename] [-r] <provider1> [provider2 ... ]\n");
		return 0;
	}

	PCWSTR filename = nullptr;
	bool realTime = false;

	std::vector<PCWSTR> names;

	for(int i = 1; i < argc; i++) {
		if(::_wcsicmp(argv[i], L"-o") == 0) {
			if(argc == i + 1) {
				printf("Missing file name\n");
				return 1;
			}
			filename = argv[i + 1];
			i++;
		}
		else if(::_wcsicmp(argv[i], L"-r") == 0) {
			realTime = true;
		}
		else {
			names.push_back(argv[i]);
		}
	}

	if(filename == nullptr && !realTime) {
		printf("You must specify -o or -r or both\n");
		return 1;
	}

	if(names.empty()) {
		printf("No providers specified\n");
		return 1;
	}

	auto providers = GetProviders(names);
	if(providers.size() < names.size()) {
		printf("Not all providers found");
		return 1;
	}

	if(!RunSession(providers, filename, realTime)) {
		printf("Failed to run session\n");
		return 1;
	}

	return 0;
}

bool RunSession(const std::vector<GUID>& providers, PCWSTR filename, bool realTime) {
	// {7791FF3D-2E25-4C3B-B90D-E33D4ADA8A36}
	static const GUID sessionGuid =
	{ 0x7791ff3d, 0x2e25, 0x4c3b, { 0xb9, 0xd, 0xe3, 0x3d, 0x4a, 0xda, 0x8a, 0x36 } };

	const WCHAR sessionName[] = L"Chapter20Session";

	auto size = sizeof(EVENT_TRACE_PROPERTIES)
		+ (filename ? ((::wcslen(filename) + 1) * sizeof(WCHAR)) : 0)
		+ sizeof(sessionName);

	auto buffer = std::make_unique<BYTE[]>(size);
	if(!buffer)
		return false;

	auto props = reinterpret_cast<EVENT_TRACE_PROPERTIES*>(buffer.get());
	DWORD status;
	TRACEHANDLE hTrace = 0;

	do {
		::ZeroMemory(buffer.get(), size);

		props->Wnode.BufferSize = (ULONG)size;
		props->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
		props->Wnode.ClientContext = 1;	// QueryPerformanceCounter
		props->Wnode.Guid = sessionGuid;
		props->LogFileMode = (filename ? EVENT_TRACE_FILE_MODE_SEQUENTIAL : 0) | (realTime ? EVENT_TRACE_REAL_TIME_MODE : 0);
		props->MaximumFileSize = 100;	// 100 MB
		props->LoggerNameOffset = sizeof(*props);
		props->LogFileNameOffset = filename ? sizeof(*props) + sizeof(sessionName) : 0;

		// copy session name
		::wcscpy_s((PWSTR)(props + 1), ::wcslen(sessionName) + 1, sessionName);

		// copy filename
		if(filename)
			::wcscpy_s((PWSTR)(buffer.get() + sizeof(*props) + sizeof(sessionName)), ::wcslen(filename) + 1, filename);

		status = ::StartTrace(&hTrace, sessionName, props);
		if(status == ERROR_ALREADY_EXISTS) {
			status = ::ControlTrace(hTrace, sessionName, props, EVENT_TRACE_CONTROL_STOP);
			continue;
		}
		break;
	} while(true);

	if(ERROR_SUCCESS != status)
		return false;

	TRACEHANDLE hParse = 0;
	HANDLE hThread = nullptr;

	if(realTime) {
		g_hStop = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);

		EVENT_TRACE_LOGFILE etl{};
		etl.LoggerName = (PWSTR)sessionName;
		etl.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_REAL_TIME;
		etl.EventRecordCallback = OnEvent;
		hParse = ::OpenTrace(&etl);
		if(hParse == INVALID_PROCESSTRACE_HANDLE) {
			printf("Failed to open a read-time session\n");
		}
		else {
			hThread = ::CreateThread(nullptr, 0, [](auto param) -> DWORD {
				FILETIME now;
				::GetSystemTimeAsFileTime(&now);
				::ProcessTrace(static_cast<TRACEHANDLE*>(param), 1, &now, nullptr);
				return 0;
				}, &hParse, 0, nullptr);

		}
	}

	for(auto& guid : providers) {
		status = ::EnableTraceEx(&guid, nullptr, hTrace, TRUE, 
			TRACE_LEVEL_VERBOSE, 0, 0, 
			EVENT_ENABLE_PROPERTY_STACK_TRACE, nullptr);
		if(ERROR_SUCCESS != status) {
			::StopTrace(hTrace, sessionName, props);
			return false;
		}
	}

	if(realTime) {
		::SetConsoleCtrlHandler([](auto code) {
			if(code == CTRL_C_EVENT) {
				::SetEvent(g_hStop);
				return TRUE;
			}
			return FALSE;
			}, TRUE);
		::WaitForSingleObject(g_hStop, INFINITE);
		::CloseTrace(hParse);
		::WaitForSingleObject(hThread, INFINITE);
		::CloseHandle(g_hStop);
		::CloseHandle(hThread);
	}
	else {
		printf("Session running... press ENTER to stop\n");

		char dummy[4];
		gets_s(dummy);
	}

	::StopTrace(hTrace, sessionName, props);

	return true;
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
			if(pi.nonStructType.InType == TDH_INTYPE_BINARY && pi.nonStructType.OutType == TDH_OUTTYPE_IPV6)
				len = sizeof(IN6_ADDR);

			if(pi.Flags & PropertyParamLength) {
				// property length is stored elsewhere
				auto index = pi.lengthPropertyIndex;
				PROPERTY_DATA_DESCRIPTOR desc;
				desc.ArrayIndex = ULONG_MAX;
				desc.PropertyName = (ULONGLONG)propName;
				desc.Reserved = 0;
				::TdhGetPropertySize(rec, 0, nullptr, 1, &desc, &len);
			}

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

