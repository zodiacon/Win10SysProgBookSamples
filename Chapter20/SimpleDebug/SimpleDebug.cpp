// SimpleDebug.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <string>
#include <memory>

HANDLE hProcess;

DWORD HandleEvent(const DEBUG_EVENT& evt);
PCSTR EventCodeToString(DWORD code);
void DisplayCreateProcessInfo(const CREATE_PROCESS_DEBUG_INFO& info);
void DisplayCreateThreadInfo(const CREATE_THREAD_DEBUG_INFO& info);
void DisplayExitThreadInfo(const EXIT_THREAD_DEBUG_INFO& info);
void DisplayLoadDllInfo(const LOAD_DLL_DEBUG_INFO& info);
void DisplayExceptionInfo(const EXCEPTION_DEBUG_INFO& info);
void DisplayUnloadDllInfo(const UNLOAD_DLL_DEBUG_INFO& info);
void DisplayOutputDebugString(const OUTPUT_DEBUG_STRING_INFO& info);
void DisplayRipInfo(const RIP_INFO& info);

int Error(const char* msg) {
	printf("%s (%u)\n", msg, ::GetLastError());
	return 1;
}

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2) {
		printf("Usage: SimpleDebug <pid | executable [args...]>\n");
		return 1;
	}

	DWORD pid = _wtoi(argv[1]);
	if (pid != 0) {
		if (!::DebugActiveProcess(pid))
			return Error("Failed to attach to process");
		printf("Attached to process %u\n", pid);
	}
	else {
		PROCESS_INFORMATION pi;
		STARTUPINFO si = { sizeof(si) };

		// build command line
		std::wstring path;
		for (int i = 1; i < argc; i++) {
			path += argv[i];
			path += L" ";
		}

		if (!::CreateProcess(nullptr, (PWSTR)path.data(), nullptr, nullptr, FALSE, DEBUG_PROCESS, nullptr, nullptr, &si, &pi))
			return Error("Failed to create and/or attach to process");
		printf("Process %u created\n", pi.dwProcessId);
		::CloseHandle(pi.hProcess);
		::CloseHandle(pi.hThread);
	}

	DEBUG_EVENT evt;
	while (::WaitForDebugEventEx(&evt, INFINITE)) {
		auto status = HandleEvent(evt);
		if (evt.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
			break;
		::ContinueDebugEvent(evt.dwProcessId, evt.dwThreadId, status);
	}

	return 0;
}

PCSTR EventCodeToString(DWORD code) {
	switch (code) {
		case CREATE_PROCESS_DEBUG_EVENT: return "Create Process";
		case CREATE_THREAD_DEBUG_EVENT: return "Create Thread";
		case EXCEPTION_DEBUG_EVENT: return "Exception";
		case EXIT_PROCESS_DEBUG_EVENT: return "Exit Process";
		case EXIT_THREAD_DEBUG_EVENT: return "Exit Thread";
		case LOAD_DLL_DEBUG_EVENT: return "Load DLL";
		case UNLOAD_DLL_DEBUG_EVENT: return "Unload DLL";
		case OUTPUT_DEBUG_STRING_EVENT: return "Output Debug String";
		case RIP_EVENT: return "RIP";
	}
	return "(Unknown)";
}

DWORD ReadImagePath(HANDLE hProcess, void* address, void* buffer, DWORD size) {
	SIZE_T read = 0;
	if (address) {
		void* p = nullptr;
		::ReadProcessMemory(hProcess, address, &p, sizeof(p), nullptr);
		if (p) {
			::ReadProcessMemory(hProcess, p, buffer, size, &read);
		}
	}
	return (DWORD)read;
}


void DisplayCreateProcessInfo(const CREATE_PROCESS_DEBUG_INFO& info) {
	printf("\tImage Base: 0x%p\n", info.lpBaseOfImage);
	printf("\tStart address: 0x%p\n", info.lpStartAddress);
	printf("\tTEB: 0x%p\n", info.lpThreadLocalBase);
	BYTE buffer[1024];
	if (ReadImagePath(hProcess, info.lpImageName, buffer, sizeof(buffer)) > 0) {
		if (info.fUnicode)
			printf("\tName: %ws\n", (PCWSTR)buffer);
		else
			printf("\tName: %s\n", (PCSTR)buffer);
	}
}

void DisplayCreateThreadInfo(const CREATE_THREAD_DEBUG_INFO& info) {
	printf("\tTEB: 0x%p\n", info.lpThreadLocalBase);
	printf("\tStart address: 0x%p\n", info.lpStartAddress);
}

void DisplayExitProcessInfo(const EXIT_PROCESS_DEBUG_INFO& info) {
	printf("\tExit code: 0x%u\n", info.dwExitCode);
}

void DisplayExitThreadInfo(const EXIT_THREAD_DEBUG_INFO& info) {
	printf("\tExit code: 0x%u\n", info.dwExitCode);
}

void DisplayLoadDllInfo(const LOAD_DLL_DEBUG_INFO& info) {
	printf("\tBase address: 0x%p\n", info.lpBaseOfDll);
	printf("\tImage Name: 0x%p\n", info.lpImageName);
	BYTE buffer[1 << 10];
	if (ReadImagePath(hProcess, info.lpImageName, buffer, sizeof(buffer)) > 0) {
		if (info.fUnicode)
			printf("\tName: %ws\n", (PCWSTR)buffer);
		else
			printf("\tName: %s\n", (PCSTR)buffer);
	}
}

void DisplayUnloadDllInfo(const UNLOAD_DLL_DEBUG_INFO& info) {
	printf("\tBase address: 0x%p\n", info.lpBaseOfDll);
}

void DisplayOutputDebugString(const OUTPUT_DEBUG_STRING_INFO& info) {
	auto buffer = std::make_unique<BYTE[]>(info.nDebugStringLength);
	SIZE_T read = 0;
	::ReadProcessMemory(hProcess, info.lpDebugStringData, buffer.get(), info.nDebugStringLength, &read);
	if (read) {
		if (info.fUnicode)
			printf("\t%ws\n", (PCWSTR)buffer.get());
		else
			printf("\t%s\n", (PCSTR)buffer.get());
	}
}

void DisplayExceptionInfo(const EXCEPTION_DEBUG_INFO& info) {
	printf("\tCode: 0x%X %s\n", info.ExceptionRecord.ExceptionCode, info.dwFirstChance ? "(First chance)" : "(Second chance)");
}

void DisplayRipInfo(const RIP_INFO& info) {
	printf("\tError: %u\tType: %u\n", info.dwError, info.dwType);
}

DWORD HandleEvent(const DEBUG_EVENT& evt) {
	printf("Event PID: %u TID: %u %s (%u)\n",
		evt.dwProcessId, evt.dwThreadId,
		EventCodeToString(evt.dwDebugEventCode), evt.dwDebugEventCode);

	switch (evt.dwDebugEventCode) {
		case CREATE_PROCESS_DEBUG_EVENT:
			DisplayCreateProcessInfo(evt.u.CreateProcessInfo);
			hProcess = evt.u.CreateProcessInfo.hProcess;
			break;

		case CREATE_THREAD_DEBUG_EVENT:
			DisplayCreateThreadInfo(evt.u.CreateThread);
			break;

		case EXIT_PROCESS_DEBUG_EVENT:
			DisplayExitProcessInfo(evt.u.ExitProcess);
			break;

		case EXIT_THREAD_DEBUG_EVENT:
			DisplayExitThreadInfo(evt.u.ExitThread);
			break;

		case LOAD_DLL_DEBUG_EVENT:
			DisplayLoadDllInfo(evt.u.LoadDll);
			break;

		case UNLOAD_DLL_DEBUG_EVENT:
			DisplayUnloadDllInfo(evt.u.UnloadDll);
			break;

		case OUTPUT_DEBUG_STRING_EVENT:
			DisplayOutputDebugString(evt.u.DebugString);
			break;

		case EXCEPTION_DEBUG_EVENT:
			DisplayExceptionInfo(evt.u.Exception);
			return DBG_EXCEPTION_NOT_HANDLED;

		case RIP_EVENT:
			DisplayRipInfo(evt.u.RipInfo);
			break;
	}
	return DBG_CONTINUE;
}
