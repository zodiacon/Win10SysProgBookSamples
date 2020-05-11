#pragma once

extern "C" bool __stdcall IsPrime(int n);

class __declspec(dllexport) PrimeCalculator {
public:
	bool IsPrime(int n) const;
};
