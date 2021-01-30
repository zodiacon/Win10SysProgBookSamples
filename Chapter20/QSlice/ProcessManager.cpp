#include "pch.h"
#include "ProcessManager.h"
#include "PerfCountersQuery.h"
#include <assert.h>


std::vector<std::shared_ptr<ProcessInfo>> ProcessManager::EnumProcesses() {
	_processes.clear();
	_processes.reserve(512);

	_cpuQuery.Clear();
	//
	// enumerate processes using perf counters
	//
	unique_pdh_query hQuery;
	if (ERROR_SUCCESS != ::PdhOpenQuery(nullptr, 0, hQuery.addressof()))
		return _processes;
	
	unique_pdh_counter hCounter;
	::PdhAddEnglishCounter(hQuery.get(), L"\\Process(*)\\ID Process", 0, &hCounter);
	::PdhCollectQueryData(hQuery.get());

	DWORD size = 0;
	DWORD count;
	::PdhGetFormattedCounterArray(hCounter.get(), PDH_FMT_LONG, &size, &count, nullptr);
	size += 1024;	// in case more processes created
	auto buffer = std::make_unique<WCHAR[]>(size);
	auto data = (PPDH_FMT_COUNTERVALUE_ITEM)buffer.get();
	::PdhGetFormattedCounterArray(hCounter.get(), PDH_FMT_LONG, &size, &count, data);

	//for (DWORD i = 0; i < count; i++) {
	//	auto& p = data[i];
	//	DWORD pid = p.FmtValue.longValue;
	//	if (pid > 0) {
	//		auto pi = std::make_shared<ProcessInfo>();
	//		pi->Pid = pid;
	//		pi->Name = p.szName;
	//		pi->CounterIndex = _cpuQuery.AddCounter((path.substr(0, pos + 1) + L"% Processor Time").c_str());
	//		_processes.push_back(std::move(pi));
	//	}
	//}
	//_cpuQuery.QueryNext();

	return _processes;
}

void ProcessManager::Update() {
	_cpuQuery.QueryNext();
	int i = 0;
	for (auto& pi : _processes)
		pi->CPU = _cpuQuery.GetValue<double>(i++);
}

const std::vector<std::shared_ptr<ProcessInfo>>& ProcessManager::GetProcesses() const {
	return _processes;
}

