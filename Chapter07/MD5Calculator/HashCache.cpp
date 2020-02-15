#include "stdafx.h"
#include "HashCache.h"
#include "..\ThreadingHelpers\AutoCriticalSection.h"

HashCache::HashCache() {
	_cache.reserve(512);
}

bool HashCache::Add(PCWSTR path, const Hash& hash) {
	AutoCriticalSection locker(_lock);
	auto it = _cache.find(path);
	if (it == _cache.end()) {
		_cache.insert({ path, hash });
		return true;
	}
	return false;
}

const Hash HashCache::Get(PCWSTR path) const {
	AutoCriticalSection locker(_lock);
	auto it = _cache.find(path);
	return it == _cache.end() ? Hash() : it->second;
}

bool HashCache::Remove(PCWSTR path) {
	AutoCriticalSection locker(_lock);
	auto it = _cache.find(path);
	if (it != _cache.end()) {
		_cache.erase(it);
		return true;
	}
	return false;
}

void HashCache::Clear() {
	AutoCriticalSection locker(_lock);
	_cache.clear();
}
