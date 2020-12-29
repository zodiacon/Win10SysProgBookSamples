#pragma once

#include "Logger.h"
#include <mutex>

class FileLogger : public Logger<FileLogger> {
public:
	explicit FileLogger(const wchar_t* filename);
	~FileLogger();

	operator bool() const {
		return _hFile != INVALID_HANDLE_VALUE;
	}

	void DoWrite(const wchar_t* buffer);

private:
	HANDLE _hFile;
	std::mutex _lock;
};

