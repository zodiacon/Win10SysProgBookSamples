#include "pch.h"
#include "Simple.h"
#include <cmath>

bool __stdcall IsPrime(int n) {
	int limit = (int)::sqrt(n);
	for (int i = 2; i <= limit; i++)
		if (n % i == 0)
			return false;
	return true;
}

bool PrimeCalculator::IsPrime(int n) const {
	return ::IsPrime(n);
}
