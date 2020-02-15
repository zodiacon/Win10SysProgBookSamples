#pragma once

#include "..\ThreadingHelpers\CriticalSection.h"

using Hash = std::vector<uint8_t>;

class HashCache {
public:
	HashCache();

	bool Add(PCWSTR path, const Hash& hash);
	const Hash Get(PCWSTR path) const;
	bool Remove(PCWSTR path);
	void Clear();

private:
	mutable CriticalSection _lock;
	std::unordered_map<std::wstring, Hash> _cache;
};

