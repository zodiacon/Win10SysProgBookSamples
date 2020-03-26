#include <Windows.h>
#include <stdio.h>

CRITICAL_SECTION cs1, cs2;

DWORD WINAPI Thread1(PVOID) {
	printf("Thread1 %u\n", ::GetCurrentThreadId());
	::EnterCriticalSection(&cs1);
	::Sleep(1000);
	::EnterCriticalSection(&cs2);
	::Sleep(1000);
	::LeaveCriticalSection(&cs2);
	::LeaveCriticalSection(&cs1);

	return 0;
}

DWORD WINAPI Thread2(PVOID) {
	printf("Thread2 %u\n", ::GetCurrentThreadId());
	::EnterCriticalSection(&cs2);
	::Sleep(1000);
	::EnterCriticalSection(&cs1);
	::Sleep(1000);
	::LeaveCriticalSection(&cs1);
	::LeaveCriticalSection(&cs2);

	return 0;
}

int main() {
	::InitializeCriticalSection(&cs1);
	::InitializeCriticalSection(&cs2);

	HANDLE hThread[2];
	hThread[0] = ::CreateThread(nullptr, 0, Thread1, nullptr, 0, nullptr);
	hThread[1] = ::CreateThread(nullptr, 0, Thread2, nullptr, 0, nullptr);

	printf("Waiting for threads to exit...\n");
	::WaitForMultipleObjects(2, hThread, TRUE, INFINITE);
}

