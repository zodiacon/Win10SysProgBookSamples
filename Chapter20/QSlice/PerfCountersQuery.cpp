#include "pch.h"
#include "PerfCountersQuery.h"
#include <assert.h>

#pragma comment(lib, "pdh")


PerfCountersQuery::PerfCountersQuery() {
	Open();
}

int PerfCountersQuery::AddCounter(const wchar_t* counterPath) {
	if (::PdhValidatePath(counterPath) != ERROR_SUCCESS)
		return - 1;

	CounterInfo info;
	auto status = ::PdhAddEnglishCounter(_query.get(), counterPath, 0, info.hCounter.addressof());
	if (status != ERROR_SUCCESS)
		return -1;

	info.Path = counterPath;
	_counters.push_back(std::move(info));
	return (int)_counters.size() - 1;
}

bool PerfCountersQuery::RemoveCounter(int index) {
	if (index < 0 || index >= _counters.size())
		return false;

	_counters.erase(_counters.begin() + index);
	return true;
}

const std::wstring& PerfCountersQuery::GetCounterPath(int index) const {
	static std::wstring empty;
	if (index < 0 || index >= _counters.size())
		return empty;
	return _counters[index].Path;
}

void PerfCountersQuery::Clear() {
	_counters.clear();
}

bool PerfCountersQuery::Open() {
	if (ERROR_SUCCESS != ::PdhOpenQuery(nullptr, 0, _query.addressof()))
		return false;
	_counters.reserve(4);
	return true;
}

bool PerfCountersQuery::Close() {
	_counters.clear();
	_query.reset();
	return true;
}

bool PerfCountersQuery::QueryNext() {
	return ::PdhCollectQueryData(_query.get()) == ERROR_SUCCESS;
}
