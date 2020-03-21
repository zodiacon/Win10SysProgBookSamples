// SimpleTimer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

void CALLBACK OnTimer(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER timer) {
	printf("TID: %u Ticks: %u\n", ::GetCurrentThreadId(), ::GetTickCount());
}

void CALLBACK OnWait(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WAIT wait, TP_WAIT_RESULT) {
	printf("Wait done TID: %u Ticks: %u\n", ::GetCurrentThreadId(), ::GetTickCount());
}

int main() {
	auto timer = ::CreateThreadpoolTimer(OnTimer, nullptr, nullptr);
	if (!timer) {
		printf("Failed to create a thread pool timer (%u)", ::GetLastError());
		return 1;
	}
	static_assert(sizeof(LONG64) == sizeof(FILETIME), "something weird!");

	LONG64 interval;
	interval = -10000 * 1000LL;
	::SetThreadpoolTimer(timer, (FILETIME*)&interval, 1000, 0);

	printf("Main thread ID: %u\n", ::GetCurrentThreadId());

	::Sleep(10000);
	::WaitForThreadpoolTimerCallbacks(timer, TRUE);
	::CloseThreadpoolTimer(timer);

	return 0;
}
