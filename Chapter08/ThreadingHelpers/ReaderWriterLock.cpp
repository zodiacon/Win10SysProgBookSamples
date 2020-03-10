#include "pch.h"
#include "ReaderWriterLock.h"

ReaderWriterLock::ReaderWriterLock() {
	::InitializeSRWLock(this);
}

void ReaderWriterLock::LockShared() {
	::AcquireSRWLockShared(this);
}

void ReaderWriterLock::UnlockShared() {
	::ReleaseSRWLockShared(this);
}

void ReaderWriterLock::LockExclusive() {
	::AcquireSRWLockExclusive(this);
}

void ReaderWriterLock::UnlockExclusive() {
	::ReleaseSRWLockExclusive(this);
}

