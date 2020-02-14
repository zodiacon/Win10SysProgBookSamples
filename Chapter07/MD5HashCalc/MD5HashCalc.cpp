// MD5HashCalc.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include "..\HashCalc\MD5Calculator.h"

void DisplayHash(const std::vector<uint8_t>& hash) {
	for(auto b : hash)
		printf("%02X", b);
	printf("\n");
}

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2) {
		printf("Usage: md5hashcalc <path>\n");
		return 0;
	}

	auto hash = MD5Calculator::Calculate(argv[1]);
	if (hash.empty()) {
		printf("Error: %d\n", ::GetLastError());
	}
	else {
		DisplayHash(hash);
	}
}

