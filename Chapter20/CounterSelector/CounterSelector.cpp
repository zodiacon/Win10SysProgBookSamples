// CounterSelector.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <Pdh.h>
#include <pdhmsg.h>
#include <stdio.h>
#include <vector>
#include <memory>
#include <string>

#pragma comment(lib, "pdh")

PDH_STATUS CALLBACK SelectorCallback(DWORD_PTR context);

std::vector<std::wstring> counters;

int main() {
	PDH_BROWSE_DLG_CONFIG config = { 0 };
	auto size = 16 << 10;	// 16 KB
	auto buffer = std::make_unique<WCHAR[]>(size);
	WCHAR title[] = L"Select Counters";
	config.bDisableMachineSelection = FALSE;
	config.bLocalCountersOnly = TRUE;
	config.bSingleCounterPerDialog = TRUE;

	// BUG: setting to TRUE does not cause a crash with <All Instances> but nothing is reported as selected
	config.bSingleCounterPerAdd = FALSE;	
	config.bIncludeInstanceIndex = TRUE;
	//config.bWildCardInstances = TRUE;
	config.bHideDetailBox = TRUE;
	config.szReturnPathBuffer = buffer.get();
	config.cchReturnPathLength = size;
	config.pCallBack = SelectorCallback;
	config.dwCallBackArg = reinterpret_cast<DWORD_PTR>(&config);
	config.szDialogBoxCaption = title;
	config.szDataSource = nullptr;
	config.dwDefaultDetailLevel = PERF_DETAIL_EXPERT;

	auto error = ::PdhBrowseCounters(&config);
	if (ERROR_SUCCESS != error && PDH_DIALOG_CANCELLED != error) {
		printf("Error invoking dialog (%u)\n", error);
		return 1;
	}

	printf("Counters selected:\n");
	for (auto& counter : counters)
		printf("%ws\n", counter.c_str());

	return 0;
}

PDH_STATUS CALLBACK SelectorCallback(DWORD_PTR context) {
	auto config = reinterpret_cast<PDH_BROWSE_DLG_CONFIG*>(context);
	if (config->CallBackStatus == ERROR_SUCCESS) {
		auto buffer = config->szReturnPathBuffer;
		for (auto p = buffer; *p; p += wcslen(p) + 1) {
			counters.push_back(p);
		}
	}
	return config->CallBackStatus;
}

