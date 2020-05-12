
// SimplePrimes.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <cstdio>
#include <Windows.h>
#include "..\SimpleDll\Simple.h"

//#ifdef _WIN64
//#pragma comment(lib, "../x64/Debug/SimpleDll.lib")
//#else
//#pragma comment(lib, "../Debug/SimpleDll.lib")
//#endif

//using PIsPrime = bool (__stdcall *)(int);
typedef bool(__stdcall* PIsPrime)(int);

int main() {
	auto hPrimesLib = ::LoadLibrary(L"SimpleDll.dll");
	if (hPrimesLib) {
		// DLL found
		auto IsPrime = (PIsPrime)::GetProcAddress(hPrimesLib, "IsPrime");
		if (IsPrime) {
			bool test = IsPrime(17);
			printf("%d\n", (int)test);
		}

	}

	//bool test = IsPrime(17);
	//printf("%d\n", (int)test);

	//PrimeCalculator calc;
	//printf("123 prime? %s\n", calc.IsPrime(123) ? "Yes" : "No");

	::FreeLibrary(hPrimesLib);

	return 0;
}
