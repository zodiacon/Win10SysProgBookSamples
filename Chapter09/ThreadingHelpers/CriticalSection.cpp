#include "pch.h"
#include "CriticalSection.h"

CriticalSection::CriticalSection(DWORD spinCount, DWORD flags) {
	::InitializeCriticalSectionEx(this, (DWORD)spinCount, flags);
}

CriticalSection::~CriticalSection() {
	::DeleteCriticalSection(this);
}

void CriticalSection::Lock() {
	::EnterCriticalSection(this);
}

void CriticalSection::Unlock() {
	::LeaveCriticalSection(this);
}

bool CriticalSection::TryLock() {
	return ::TryEnterCriticalSection(this);
}
