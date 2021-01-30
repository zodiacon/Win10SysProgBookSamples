#pragma once

#include <Pdh.h>
#include <wil\resource.h>

using unique_pdh_query = wil::unique_any_handle_null_only<decltype(&::PdhCloseQuery), ::PdhCloseQuery>;
using unique_pdh_counter = wil::unique_any_handle_null_only<decltype(&::PdhRemoveCounter), ::PdhRemoveCounter>;

class PerfCountersQuery {
public:
	PerfCountersQuery();

	int AddCounter(const wchar_t* counterPath);
	bool RemoveCounter(int index);
	const std::wstring& GetCounterPath(int index) const;
	void Clear();
	bool Open();
	bool Close();
	bool QueryNext();

	template<typename T>
	T GetValue(int index) const;

	template<>
	double GetValue(int index) const {
		_value.doubleValue = std::numeric_limits<double>::quiet_NaN();
		::PdhGetFormattedCounterValue(_counters[index].hCounter.get(),
			PDH_FMT_DOUBLE | PDH_FMT_NOSCALE | PDH_FMT_NOCAP100,
			nullptr, &_value);
		return _value.doubleValue;
	}

	template<>
	LONG GetValue(int index) const {
		_value.longValue = std::numeric_limits<LONG>::quiet_NaN();
		::PdhGetFormattedCounterValue(_counters[index].hCounter.get(),
			PDH_FMT_LONG | PDH_FMT_NOSCALE | PDH_FMT_NOCAP100,
			nullptr, &_value);
		return _value.longValue;
	}

	template<>
	long long GetValue(int index) const {
		_value.largeValue = std::numeric_limits<LONGLONG>::quiet_NaN();
		::PdhGetFormattedCounterValue(_counters[index].hCounter.get(),
			PDH_FMT_LARGE | PDH_FMT_NOSCALE | PDH_FMT_NOCAP100,
			nullptr, &_value);
		return _value.largeValue;
	}

private:
	struct CounterInfo {
		std::wstring Path;
		unique_pdh_counter hCounter;
	};
	mutable PDH_FMT_COUNTERVALUE _value;
	unique_pdh_query _query;
	std::vector<CounterInfo> _counters;
};

