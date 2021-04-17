// RunETW.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <tdh.h>
#include <vector>
#include <assert.h>
#include <memory>

#pragma comment(lib, "tdh")

bool RunSession(const std::vector<GUID>& providers, PCWSTR filename, bool realTime);

std::vector<GUID> GetProviders(std::vector<PCWSTR> names) {
	std::vector<GUID> providers;

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

	// build a vector of providers
	providers.reserve(data->NumberOfProviders);
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
		printf("Usage: RunETW [-o filename] [-r] <provider1> [provider2 ... ]\n");
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
	::ZeroMemory(buffer.get(), size);

	auto props = reinterpret_cast<EVENT_TRACE_PROPERTIES*>(buffer.get());

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

	TRACEHANDLE hTrace = 0;
	auto status = ::StartTrace(&hTrace, sessionName, props);
	if(ERROR_SUCCESS != status)
		return false;

	for(auto& guid : providers) {	
		status = ::EnableTraceEx(&guid, nullptr, hTrace, TRUE, TRACE_LEVEL_INFORMATION, 0, 0, EVENT_ENABLE_PROPERTY_STACK_TRACE, nullptr);
		if(ERROR_SUCCESS != status) {
			::StopTrace(hTrace, sessionName, props);
			return false;
		}
	}

	printf("Session running... press ENTER to stop\n");

	char dummy[4];
	gets_s(dummy);

	::StopTrace(hTrace, sessionName, props);

	return true;
}
