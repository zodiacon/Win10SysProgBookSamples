// QSlice.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <wil\resource.h>
#include <Pdh.h>
#include <assert.h>

#pragma comment(lib, "pdh")

using unique_pdh_query = wil::unique_any_handle_null_only<decltype(&::PdhCloseQuery), ::PdhCloseQuery>;
using unique_pdh_counter = wil::unique_any_handle_null_only<decltype(&::PdhRemoveCounter), ::PdhRemoveCounter>;

#define RET_IF_ERR(error) if(ERROR_SUCCESS != (error)) return error;

struct ProcessInfo {
	DWORD Pid;
	std::wstring Name;
	double CPU;
};

int main(int argc, const char* argv[]) {
	auto display = argc > 1 ? atoi(argv[1]) : 10;
	if (display < 5)
		display = 5;

	unique_pdh_query hQuery;
	RET_IF_ERR(::PdhOpenQuery(nullptr, 0, hQuery.addressof()));

	unique_pdh_counter hPidCounter, hCpuCounter;
	RET_IF_ERR(::PdhAddEnglishCounter(hQuery.get(), L"\\Process(*)\\ID Process", 0, hPidCounter.addressof()));
	RET_IF_ERR(::PdhAddEnglishCounter(hQuery.get(), L"\\Process(*)\\% Processor Time", 0, hCpuCounter.addressof()));

	RET_IF_ERR(::PdhCollectQueryData(hQuery.get()));
	RET_IF_ERR(::PdhCollectQueryData(hQuery.get()));

	auto processors = ::GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);

	auto hConsole = ::GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info = { sizeof(info) };
	::GetConsoleScreenBufferInfo(hConsole, &info);
	info.dwSize.X = 150;
	::SetConsoleScreenBufferSize(hConsole, info.dwSize);

	for (;;) {
		DWORD size = 0;
		DWORD count;
		::PdhGetFormattedCounterArray(hPidCounter.get(), PDH_FMT_LONG, &size, &count, nullptr);
		auto pidBuffer = std::make_unique<BYTE[]>(size);
		auto pidData = (PPDH_FMT_COUNTERVALUE_ITEM)pidBuffer.get();
		::PdhGetFormattedCounterArray(hPidCounter.get(), PDH_FMT_LONG, &size, &count, pidData);

		size = 0;
		DWORD count2;
		::PdhGetFormattedCounterArray(hCpuCounter.get(), PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, &size, &count2, nullptr);
		auto cpuBuffer = std::make_unique<BYTE[]>(size);
		auto cpuData = (PPDH_FMT_COUNTERVALUE_ITEM)cpuBuffer.get();
		::PdhGetFormattedCounterArray(hCpuCounter.get(), PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, &size, &count2, cpuData);

		assert(count == count2);

		std::vector<ProcessInfo> processes;
		processes.reserve(count);
		for (DWORD i = 0; i < count; i++) {
			auto& p = pidData[i];
			auto& p2 = cpuData[i];
			if (p.FmtValue.longValue == 0)
				continue;

			ProcessInfo pi;
			assert(::_wcsicmp(p.szName, p2.szName) == 0);
			pi.Name = p.szName;
			pi.Pid = p.FmtValue.longValue;
			pi.CPU = p2.FmtValue.doubleValue;
			processes.push_back(std::move(pi));
		}
		std::sort(processes.begin(), processes.end(), [](const auto& p1, const auto& p2) {
			return p1.CPU > p2.CPU;
			});

		for (int i = 0; i < display; i++) {
			auto& p = processes[i];
			auto cpu = p.CPU;
			int blocks = (int)(cpu / processors + .5);
			printf("%6d %-30ws %6.2f%% %s%s\n",
				p.Pid, p.Name.c_str(), cpu / processors,
				std::string(blocks, (char)219).c_str(),
				std::string(101 - blocks, ' ').c_str());
		}
		::Sleep(1000);
		printf("\r\033[%dA", display);
		RET_IF_ERR(::PdhCollectQueryData(hQuery.get()));
	}
	return 0;
}

