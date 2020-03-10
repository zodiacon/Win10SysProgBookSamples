// SimpleTimer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

void CALLBACK OnTimer(void* param, DWORD low, DWORD high) {
	printf("TID: %u Ticks: %u\n", ::GetCurrentThreadId(), ::GetTickCount());
}

int main() {
	auto hTimer = ::CreateWaitableTimer(nullptr, TRUE, nullptr);
	LARGE_INTEGER interval;
	interval.QuadPart = -10000 * 1000LL;
	::SetWaitableTimer(hTimer, &interval, 1000, OnTimer, nullptr, FALSE);
	printf("Main thread ID: %u\n", ::GetCurrentThreadId());

	while (true)
		::SleepEx(INFINITE, TRUE);
	return 0;
}
