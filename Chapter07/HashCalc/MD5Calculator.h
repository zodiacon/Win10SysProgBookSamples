#pragma once

#include <vector>

class MD5Calculator {
public:
	static std::vector<uint8_t> Calculate(PCWSTR path);
};

