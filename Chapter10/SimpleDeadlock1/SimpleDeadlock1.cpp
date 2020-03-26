#include <Windows.h>
#include <stdio.h>

HANDLE hMutex1, hMutex2;

DWORD WINAPI Thread1(PVOID) {
	printf("Thread1 %u\n", ::GetCurrentThreadId());
	::WaitForSingleObject(hMutex1, INFINITE);
	::Sleep(1000);
	::WaitForSingleObject(hMutex2, INFINITE);
	return 0;
}

DWORD WINAPI Thread2(PVOID) {
	printf("Thread2 %u\n", ::GetCurrentThreadId());
	::WaitForSingleObject(hMutex2, INFINITE);
	::Sleep(1000);
	::WaitForSingleObject(hMutex1, INFINITE);
	return 0;
}

int main() {
	hMutex1 = ::CreateMutex(nullptr, FALSE, L"SimpleDeadlockMutex1");
	hMutex2 = ::CreateMutex(nullptr, FALSE, L"SimpleDeadlockMutex2");

	HANDLE hThread[2];
	hThread[0] = ::CreateThread(nullptr, 0, Thread1, nullptr, 0, nullptr);
	hThread[1] = ::CreateThread(nullptr, 0, Thread2, nullptr, 0, nullptr);

	printf("Waiting for threads to exit...\n");
	::WaitForMultipleObjects(2, hThread, TRUE, INFINITE);
}

