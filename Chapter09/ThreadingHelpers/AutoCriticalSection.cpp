#include "pch.h"
#include "AutoCriticalSection.h"

AutoCriticalSection::AutoCriticalSection(CRITICAL_SECTION& cs) : _cs(cs) {
	::EnterCriticalSection(&_cs);
}

AutoCriticalSection::~AutoCriticalSection() {
	::LeaveCriticalSection(&_cs);
}
