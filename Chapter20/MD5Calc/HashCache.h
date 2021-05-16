#pragma once

using Hash = std::vector<uint8_t>;

#include <unordered_map>

class HashCache {
public:
	HashCache();

	bool Add(PCWSTR path, const Hash& hash);
	const Hash Get(PCWSTR path) const;
	bool Remove(PCWSTR path);
	void Clear();

private:
	std::unordered_map<std::wstring, Hash> _cache;
};


