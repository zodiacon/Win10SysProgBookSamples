#pragma once

#include <unordered_map>
#include "PerfCountersQuery.h"

struct ProcessInfo {
	DWORD Pid;
	std::wstring Name;
	std::wstring PerfCounterPath;
	double CPU;
	int CounterIndex;
};

class ProcessManager {
public:
	std::vector<std::shared_ptr<ProcessInfo>> EnumProcesses();
	void Update();

	const std::vector<std::shared_ptr<ProcessInfo>>& GetProcesses() const;

private:
	PerfCountersQuery _cpuQuery;
	std::vector<std::shared_ptr<ProcessInfo>> _processes;
};

