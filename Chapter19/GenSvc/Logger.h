#pragma once

#include <string>
#include <strsafe.h>
#include <type_traits>

enum class LogLevel {
	Debug,
	Info,
	Warning,
	Error,
	Critical
};

template<typename TLogger>
class Logger {
public:
	template<typename... Args>
	void Write(LogLevel level, const wchar_t* format, Args&&... args) {
		WCHAR buffer[512];
		::StringCchPrintf(buffer, _countof(buffer), 
			(std::wstring(L"%s,%s,") + format + L"\n").c_str(), LogLevelToString(level), TimeToString().c_str(), std::forward<Args>(args)...);
		static_cast<TLogger*>(this)->DoWrite(buffer);
	}

	template<typename... Args>
	void Debug(const wchar_t* format, Args&&... args) {
		Write(LogLevel::Debug, format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void Info(const wchar_t* format, Args&&... args) {
		Write(LogLevel::Info, format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void Warning(const wchar_t* format, Args&&... args) {
		Write(LogLevel::Warning, format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void Error(const wchar_t* format, Args&&... args) {
		Write(LogLevel::Error, format, std::forward<Args>(args)...);
	}

	static const wchar_t* LogLevelToString(LogLevel level) {
		static const wchar_t* levels[] = {
			L"Debug", L"Info", L"Warning", L"Error", L"Critical"
		};
		return levels[static_cast<int>(level)];
	}

protected:
	static std::wstring TimeToString() {
		WCHAR date[13], time[13];
		::_wstrdate_s(date);
		::_wstrtime_s(time);
		return std::wstring(date) + L" " + time;
	}
};

