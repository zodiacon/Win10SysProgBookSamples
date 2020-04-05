#include <stdio.h>
#include <Windows.h>
#include <memory>

void DisplayData(long long offset, const BYTE* buffer, DWORD bytes);

int Error(const char* text) {
	printf("%s (%d)\n", text, ::GetLastError());
	return 1;
}

int main(int argc, const char* argv[]) {
	if (argc < 4) {
		printf("Usage: DumpDrive <index> <offset in sectors> <size in sectors>\n");
		return 0;
	}

	WCHAR path[] = L"\\\\.\\PhysicalDriveX";
	path[::wcslen(path) - 1] = argv[1][0];

	auto offset = atoll(argv[2]) * 512;
	auto size = atol(argv[3]) * 512;

	HANDLE hDevice = ::CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE)
		return Error("Failed to open Physical drive");

	LARGE_INTEGER fp;
	fp.QuadPart = offset;
	if (!::SetFilePointerEx(hDevice, fp, nullptr, FILE_BEGIN))
		return Error("Failed in SetFilePointerEx");

	auto buffer = std::make_unique<BYTE[]>(size);
	DWORD bytes;
	if (!::ReadFile(hDevice, buffer.get(), size, &bytes, nullptr))
		return Error("Failed to read data");

	DisplayData(offset, buffer.get(), bytes);

	::CloseHandle(hDevice);

	return 0;
}

void DisplayData(long long offset, const BYTE* buffer, DWORD bytes) {
	const int bytesPerLine = 16;
	for (DWORD i = 0; i < bytes; i += bytesPerLine) {
		printf("%16X: ", offset + i);
		for (int b = 0; b < bytesPerLine; b++) {
			printf("%02X ", buffer[i + b]);
		}
		printf("\n");
	}
}
