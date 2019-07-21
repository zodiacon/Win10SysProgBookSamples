// HelloWin.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

int main() {
	SYSTEM_INFO si;
	::GetNativeSystemInfo(&si);

	printf("Number of Logical Processors: %d\n", si.dwNumberOfProcessors);
	printf("Page size: %d Bytes\n", si.dwPageSize);
	printf("Processor Mask: 0x%p\n", (PVOID)si.dwActiveProcessorMask);
	printf("Minimum process address: 0x%p\n", si.lpMinimumApplicationAddress);
	printf("Maximum process address: 0x%p\n", si.lpMaximumApplicationAddress);

	return 0;
}
