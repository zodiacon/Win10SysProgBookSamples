// SimplePrimes2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>

#include "..\SimpleDll\Simple.h"
#include <delayimp.h>

bool IsLoaded() {
	auto hModule = ::GetModuleHandle(L"simpledll");
	printf("SimpleDll loaded: %s\n", hModule ? "Yes" : "No");
	return hModule != nullptr;
}

int main() {
	IsLoaded();

	bool prime = IsPrime(17);

	IsLoaded();

	printf("17 is prime? %s\n", prime ? "Yes" : "No");
	__FUnloadDelayLoadedDLL2("SimpleDll.dll");

	IsLoaded();

	prime = IsPrime(1234567);

	IsLoaded();

	return 0;
}

