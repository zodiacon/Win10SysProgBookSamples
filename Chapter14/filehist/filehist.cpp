// filehist.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <algorithm>

int Error(const char* msg) {
	printf("%s (%u)\n", msg, ::GetLastError());
	return 1;
}

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2) {
		printf("Usage:\tfilehist [view size in MB] <file path>\n");
		printf("\tDefault view size is 10 MB\n");
		return 0;
	}

	DWORD viewSize = argc == 2 ? (10 << 20) : (_wtoi(argv[1]) << 20);
	if (viewSize == 0)
		viewSize = 10 << 20;

	struct Data {
		BYTE Value;
		long long Count;
	};

	Data count[256] = { 0 };
	for (int i = 0; i < 256; i++)
		count[i].Value = i;

	HANDLE hFile = ::CreateFile(argv[argc - 1], GENERIC_READ, FILE_SHARE_READ,
		nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return Error("Failed to open file");

	LARGE_INTEGER fileSize;
	if (!::GetFileSizeEx(hFile, &fileSize))
		return Error("Failed to get file size");

	HANDLE hMapFile = ::CreateFileMapping(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
	if (!hMapFile)
		return Error("Failed to create MMF");

	::CloseHandle(hFile);

	auto total = fileSize.QuadPart;
	printf("File size: %llu bytes\n", fileSize.QuadPart);
	printf("Using view size: %u MB\n", (unsigned)(viewSize >> 20));

	LARGE_INTEGER offset = { 0 };
	while (fileSize.QuadPart > 0) {
		auto mapSize = (unsigned)min(viewSize, fileSize.QuadPart);
		printf("Mapping offset: 0x%llX, size: 0x%X bytes\n", offset.QuadPart, mapSize);

		auto p = (const BYTE*)::MapViewOfFile(hMapFile, FILE_MAP_READ, offset.HighPart, offset.LowPart, mapSize);
		if (!p)
			return Error("Failed in MapViewOfFile");

		// do the work
		for (DWORD i = 0; i < mapSize; i++)
			count[p[i]].Count++;
		::UnmapViewOfFile(p);

		offset.QuadPart += mapSize;
		fileSize.QuadPart -= mapSize;
	}
	::CloseHandle(hMapFile);

	// sort by ascending order
	std::sort(std::begin(count), std::end(count), [](const auto& c1, const auto& c2) {
		return c2.Count > c1.Count;
		});

	// display results
	for (const auto& data : count) {
		printf("0x%02X: %10llu (%5.2f %%)\n", data.Value, data.Count, data.Count * 100.0 / total);
	}

	return 0;
}

