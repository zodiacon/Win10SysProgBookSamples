// CalculatorSvr.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <unordered_map>
#include <functional>

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

const int MaxElements = 100;
const DWORD MaxInputMessageSize = sizeof(InputMessage) + (MaxElements - 1) * sizeof(double);

void CALLBACK HandleConnection(PTP_CALLBACK_INSTANCE instance, PVOID context);

int Error(const char* msg, DWORD error = ::GetLastError()) {
	printf("%s (%u)\n", msg, error);
	return 1;
}

int main() {
	for (;;) {
		HANDLE hNamedPipe = ::CreateNamedPipe(L"\\\\.\\pipe\\calculator",
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT | PIPE_ACCEPT_REMOTE_CLIENTS,
			PIPE_UNLIMITED_INSTANCES,
			sizeof(double), MaxInputMessageSize, 0, nullptr);

		if (hNamedPipe == INVALID_HANDLE_VALUE)
			return Error("Failed to create named pipe");

		printf("Named pipe calculator created successfully. Listening...\n");

		if (!::ConnectNamedPipe(hNamedPipe, nullptr) && ::GetLastError() != ERROR_PIPE_CONNECTED)
			return Error("Failed in ConnectNamedPipe");

		::TrySubmitThreadpoolCallback(HandleConnection, hNamedPipe, nullptr);
	}

	return 0;
}

void CALLBACK HandleConnection(PTP_CALLBACK_INSTANCE instance, PVOID context) {
	BYTE buffer[MaxInputMessageSize];
	auto hPipe = static_cast<HANDLE>(context);

	static const std::unordered_map<MessageType, std::function<double(double*, ULONG)>> ops{
		{ MessageType::Sum, [](auto numbers, auto count) {
			double result = 0;
			for (ULONG i = 0; i < count; i++)
				result += numbers[i];
			return result;
			}
		},
		{ MessageType::Average, [](auto numbers, auto count) {
			double result = 0;
			for (ULONG i = 0; i < count; i++)
				result += numbers[i];
			return result / count;
			}
		},
		{ MessageType::Product, [](auto numbers, auto count) {
			double result = 1;
			for (ULONG i = 0; i < count; i++)
				result *= numbers[i];
			return result;
			}
		},
	};

	DWORD read;
	for (;;) {
		if (!::ReadFile(hPipe, buffer, sizeof(buffer), &read, nullptr) || read == 0) {
			if (::GetLastError() == ERROR_BROKEN_PIPE) {
				printf("Client disconnected\n");
			}
			else {
				printf("Error reading from pipe (%u)\n", ::GetLastError());
			}
			break;
		}
		auto msg = reinterpret_cast<InputMessage*>(buffer);
		if (read < sizeof(InputMessage) || read < (msg->Count - 1) * sizeof(double) + sizeof(InputMessage)) {
			printf("Message too short\n");
			break;
		}
		auto count = msg->Count;
		if (count > MaxElements) {
			printf("Too many elements\n");
			break;
		}
		auto it = ops.find(msg->Type);
		if (it == ops.end()) {
			printf("Unknown math operation\n");
			continue;
		}
		auto result = (it->second)(msg->Values, msg->Count);
		DWORD written;
		if (!::WriteFile(hPipe, &result, sizeof(result), &written, nullptr)) {
			printf("Failed to send result!\n");
			continue;
		}
		printf("Result: %f sent!\n", result);
	}

	::DisconnectNamedPipe(hPipe);
	::CloseHandle(hPipe);
}

