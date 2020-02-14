#pragma once

class CriticalSection : public CRITICAL_SECTION {
public:
	CriticalSection(DWORD spinCount = 0, DWORD flags = 0);
	~CriticalSection();

	void Lock();
	void Unlock();
	bool TryLock();
};

