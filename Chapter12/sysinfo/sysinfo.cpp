// sysinfo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <stdio.h>
#include <string>

void DisplaySystemInfo(const SYSTEM_INFO* si, const char* title);

int main() {
	SYSTEM_INFO si;
	::GetSystemInfo(&si);
	DisplaySystemInfo(&si, "System information");

	BOOL isWow = FALSE;
	if (sizeof(void*) == 4 && ::IsWow64Process(::GetCurrentProcess(), &isWow) && isWow) {
		::GetNativeSystemInfo(&si);
		printf("\n");
		DisplaySystemInfo(&si, "Native System information");
	}

	return 0;
}

const char* GetProcessorArchitecture(WORD arch) {
	switch (arch) {
		case PROCESSOR_ARCHITECTURE_AMD64: return "x64";
		case PROCESSOR_ARCHITECTURE_INTEL: return "x86";
		case PROCESSOR_ARCHITECTURE_ARM: return "ARM";
		case PROCESSOR_ARCHITECTURE_ARM64: return "ARM64";
	}
	return "Unknown";
}

void DisplaySystemInfo(const SYSTEM_INFO* si, const char* title) {
	printf("%s\n%s\n", title, std::string(::strlen(title), '-').c_str());
	printf("%-24s%s\n", "Processor Architecture:", GetProcessorArchitecture(si->wProcessorArchitecture));
	printf("%-24s%u\n", "Number of Processors:", si->dwNumberOfProcessors);
	printf("%-24s0x%llX\n", "Active Processor Mask:", (DWORD64)si->dwActiveProcessorMask);
	printf("%-24s%u KB\n", "Page Size:", si->dwPageSize >> 10);
	printf("%-24s0x%p\n", "Min User Space Address:", si->lpMinimumApplicationAddress);
	printf("%-24s0x%p\n", "Max User Space Address:", si->lpMaximumApplicationAddress);
	printf("%-24s%u KB\n", "Allocation Granularity:", si->dwAllocationGranularity >> 10);
}