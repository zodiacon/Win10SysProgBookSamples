// CalcClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <memory>

int Error(const char* msg, DWORD error = ::GetLastError()) {
	printf("%s (%u)\n", msg, error);
	return 1;
}

enum class MessageType {
	Sum,
	Product,
	Average,
};

struct InputMessage {
	MessageType Type;
	ULONG Count;
	double Values[ANYSIZE_ARRAY];
};

bool SendRequest(HANDLE hPipe, MessageType type, const double* values, ULONG count) {
	DWORD messageSize = sizeof(InputMessage) + sizeof(double) * (count - 1);
	auto buffer = std::make_unique<BYTE[]>(messageSize);
	auto msg = reinterpret_cast<InputMessage*>(buffer.get());
	msg->Type = type;
	msg->Count = count;
	::memcpy(msg->Values, values, sizeof(double) * count);

	DWORD written;
	if (!::WriteFile(hPipe, msg, messageSize, &written, nullptr)) {
		printf("Failed to write to pipe (%u)\n", ::GetLastError());
		return false;
	}
	::FlushFileBuffers(hPipe);

	double result;
	DWORD read;
	if (!::ReadFile(hPipe, &result, sizeof(result), &read, nullptr)) {
		printf("Failed to read result (%u)\n", ::GetLastError());
		return false;
	}

	printf("Result: %f\n", result);
	return true;
}

int main() {
	WCHAR pipeName[] = L"\\\\.\\pipe\\calculator";
	HANDLE hPipe;

	int count = 3;
	for (;;) {
		::WaitNamedPipe(pipeName, NMPWAIT_WAIT_FOREVER);
		hPipe = ::CreateFile(pipeName, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		if (hPipe != INVALID_HANDLE_VALUE)
			break;
		if (--count == 0)
			return Error("Cannot establish connection", ERROR_NOT_CONNECTED);
	}

	// test with sum data
	double values[] = { 12, 3.5, 9.3, 22, .2, 11, 37.8, -10, -6.3 };
	SendRequest(hPipe, MessageType::Sum, values, _countof(values));
	::Sleep(1000);
	SendRequest(hPipe, MessageType::Average, values, _countof(values));
	SendRequest(hPipe, MessageType::Product, values, _countof(values));

	::CloseHandle(hPipe);
	return 0;
}

