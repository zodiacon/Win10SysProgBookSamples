#include "pch.h"
#include "AutoReaderWriterLock.h"

AutoReaderWriterLockExclusive::AutoReaderWriterLockExclusive(SRWLOCK& lock) : _lock(lock) {
	::AcquireSRWLockExclusive(&_lock);
}

AutoReaderWriterLockExclusive::~AutoReaderWriterLockExclusive() {
	::ReleaseSRWLockExclusive(&_lock);
}

