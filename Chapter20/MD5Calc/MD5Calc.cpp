// MD5Calc.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "MD5Calculator.h"
#include "HashCache.h"
#include "ProcessManager.h"

// {C44DF504-CA51-4CA6-BEA6-F20B6AECAED9}
TRACELOGGING_DEFINE_PROVIDER(g_Provider, "MD5CalcProvider", 
	(0xc44df504, 0xca51, 0x4ca6, 0xbe, 0xa6, 0xf2, 0xb, 0x6a, 0xec, 0xae, 0xd9));

const char* HashToString(const std::vector<uint8_t>& v) {
	static std::string result;
	result.clear();
	char value[3];
	for(auto n : v) {
		sprintf_s(value, "%02X", n);
		result += value;
	}
	return result.c_str();
}

std::wstring GenerateEventName() {
	return L"MD5CalcStopEvent" + std::to_wstring(::GetCurrentProcessId());
}

int main() {
	::TraceLoggingRegister(g_Provider);

	TraceLoggingWrite(g_Provider, "Init", 
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingValue(L"Process started", "Text"),
		TraceLoggingValue(::GetCurrentProcessId(), "PID"));

	HashCache cache;
	ProcessManager pm;

	::SetConsoleCtrlHandler([](auto code) {
		if(code == CTRL_C_EVENT) {
			HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, GenerateEventName().c_str());
			_ASSERT(hEvent);
			::SetEvent(hEvent);
			::CloseHandle(hEvent);
			return TRUE;
		}
		return FALSE;
		}, TRUE);

	HANDLE hEvent = CreateEvent(nullptr, FALSE, FALSE, GenerateEventName().c_str());
	while(::WaitForSingleObject(hEvent, 1000) == WAIT_TIMEOUT) {
		pm.Refresh();
		printf("Received %u new processes\n", (uint32_t)pm.GetNewProcesses().size());
		for(auto pid : pm.GetNewProcesses()) {
			TraceLoggingWrite(g_Provider, "Processing",
				TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
				TraceLoggingUInt32(pid, "PID"));

			for(auto& m : pm.EnumModules(pid)) {
				auto hash = cache.Get(m.c_str());
				bool cached = true;
				if(hash.empty()) {
					hash = MD5Calculator::Calculate(m.c_str());
					cached = false;
					cache.Add(m.c_str(), hash);
				}
				printf("MD5: %s Cached: %s Path: %ws\n", HashToString(hash), cached ? "YES" : "NO ", m.c_str());
			}
		}
		::Sleep(1000);
	}
	::CloseHandle(hEvent);

	TraceLoggingWrite(g_Provider, "Term", 
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION));

	::TraceLoggingUnregister(g_Provider);
	return 0;
}

