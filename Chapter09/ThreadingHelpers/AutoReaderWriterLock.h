#pragma once

#include "ReaderWriterLock.h"

struct AutoReaderWriterLockExclusive {
	AutoReaderWriterLockExclusive(SRWLOCK& lock);
	~AutoReaderWriterLockExclusive();

private:
	SRWLOCK& _lock;
};

struct AutoReaderWriterLockShared {
	AutoReaderWriterLockShared(SRWLOCK& lock);
	~AutoReaderWriterLockShared();

private:
	SRWLOCK& _lock;
};

