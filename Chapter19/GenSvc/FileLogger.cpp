#include "pch.h"
#include "FileLogger.h"

FileLogger::FileLogger(const wchar_t* filename) {
	_hFile = ::CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, 0, nullptr);
}

FileLogger::~FileLogger() {
	if (_hFile != INVALID_HANDLE_VALUE)
		::CloseHandle(_hFile);
}

void FileLogger::DoWrite(const wchar_t* buffer) {
	std::lock_guard locker(_lock);
	::SetFilePointer(_hFile, 0, nullptr, FILE_END);
	DWORD bytes;
	::WriteFile(_hFile, buffer, DWORD(::wcslen(buffer) * sizeof(WCHAR)), &bytes, nullptr);
}
