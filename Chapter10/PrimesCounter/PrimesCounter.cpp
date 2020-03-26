// PrimesCounter.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <ppl.h>

// types

struct PrimesData {
	int From, To;
	int Count;
};

// prototypes

bool IsPrime(int n);
DWORD WINAPI CalcPrimes(PVOID param);
int CalcAllPrimes(int from, int to, int threads, DWORD& elapsed);

// main

int main(int argc, const char* argv[]) {
	if (argc < 4) {
		printf("Usage: PrimesCounter <from> <to> <threads>\n");
		return 0;
	}

	int from = atoi(argv[1]);
	int to = atoi(argv[2]);
	int threads = atoi(argv[3]);
	
	if (from < 1 || to < 1 || threads < 1 || threads > 64) {
		printf("Invalid input.\n");
		return 1;
	}

	DWORD elapsed;
	int count = CalcAllPrimes(from, to, threads, elapsed);
	printf("Total primes: %d. Elapsed: %d msec\n", count, elapsed);

	auto start = ::GetTickCount64();
	
	// use concrt
	count = 0;
	concurrency::parallel_for(from, to + 1, [&count](int n) {
		if (IsPrime(n))
			::InterlockedIncrement((unsigned*)&count);
		});
	auto end = ::GetTickCount64();
	printf("Using concrt: Primes: %d, Elapsed: %u msec\n", count, (ULONG)(end - start));

	return 0;
}

bool IsPrime(int n) {
	if (n < 3)
		return false;

	int limit = (int)::sqrt(n);
	for (int i = 2; i <= limit; i++)
		if (n % i == 0)
			return false;

	return true;
}

int CalcAllPrimes(int from, int to, int threads, DWORD& elapsed) {
	auto start = ::GetTickCount64();
	auto data = std::make_unique<PrimesData[]>(threads);
	auto handles = std::make_unique<HANDLE[]>(threads);

	int chunk = (to - from + 1) / threads;

	for (int i = 0; i < threads; i++) {
		auto& d = data[i];
		d.From = i * chunk + from;
		d.To = i == threads - 1 ? to : from + (i + 1) * chunk - 1;

		DWORD tid;
		handles[i] = ::CreateThread(nullptr, 0, CalcPrimes, &d, 0, &tid);
		assert(handles[i]);
		printf("Thread %d created (%d to %d). TID=%u\n", i + 1, d.From, d.To, tid);
	}
	::WaitForMultipleObjects(threads, handles.get(), TRUE, INFINITE);
	elapsed = static_cast<DWORD>(::GetTickCount64() - start);

	FILETIME dummy, kernel, user;
	int total = 0;
	for (int i = 0; i < threads; i++) {
		::GetThreadTimes(handles[i], &dummy, &dummy, &kernel, &user);
		int count = data[i].Count;
		printf("Thread %2d Count: %7d. Execution time: %4u msec\n", i + 1, count,
			(user.dwLowDateTime + kernel.dwLowDateTime) / 10000);
		total += count;
		::CloseHandle(handles[i]);
	}
	return total;
}

DWORD WINAPI CalcPrimes(PVOID param) {
	auto data = static_cast<PrimesData*>(param);
	int from = data->From, to = data->To;
	int count = 0;
	for (int i = from; i <= to; i++)
		if (IsPrime(i))
			count++;

	data->Count = count;
	return count;
}
