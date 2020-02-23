#pragma once

class ReaderWriterLock : public SRWLOCK {
public:
	ReaderWriterLock();
	ReaderWriterLock(const ReaderWriterLock&) = delete;
	ReaderWriterLock& operator=(const ReaderWriterLock&) = delete;

	void LockShared();
	void UnlockShared();

	void LockExclusive();
	void UnlockExclusive();
};

