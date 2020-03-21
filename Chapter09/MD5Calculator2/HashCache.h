#pragma once

#include "..\ThreadingHelpers\ReaderWriterLock.h"

using Hash = std::vector<uint8_t>;

class HashCache {
public:
	HashCache();

	bool Add(PCWSTR path, const Hash& hash);
	const Hash Get(PCWSTR path) const;
	bool Remove(PCWSTR path);
	void Clear();

private:
	mutable ReaderWriterLock _lock;
	std::unordered_map<std::wstring, Hash> _cache;
};

