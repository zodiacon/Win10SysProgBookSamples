#pragma once

struct AutoCriticalSection {
	AutoCriticalSection(CRITICAL_SECTION& cs);
	~AutoCriticalSection();

	// delete copy ctor, move ctor, assignment
	AutoCriticalSection(const AutoCriticalSection&) = delete;
	AutoCriticalSection& operator=(const AutoCriticalSection&) = delete;
	AutoCriticalSection(AutoCriticalSection&&) = delete;
	AutoCriticalSection& operator=(AutoCriticalSection&&) = delete;

private:
	CRITICAL_SECTION& _cs;
};

