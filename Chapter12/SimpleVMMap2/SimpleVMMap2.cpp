// SimpleVMMap2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <string>
#include <assert.h>
#include <algorithm>
#include <memory>

int Error(const char* message) {
	printf("%s (error=%d)\n", message, ::GetLastError());
	return 1;
}

const char* MemoryTypeToString(DWORD type) {
	switch (type) {
		case MEM_IMAGE: return "Image";
		case MEM_MAPPED: return "Mapped";
		case MEM_PRIVATE: return "Private";
	}
	assert(false);
	return "";
}

const char* StateToString(DWORD state) {
	switch (state) {
		case MEM_COMMIT: return "Committed";
		case MEM_RESERVE: return "Reserved";
		case MEM_FREE: return "Free";
	}
	assert(false);
	return "";
}

std::string ProtectionToString(DWORD protection) {
	static const struct {
		PCSTR Text;
		DWORD Value;
	} prot[] = {
		{ "", 0 },
		{ "Execute", PAGE_EXECUTE },
		{ "Execute/Read", PAGE_EXECUTE_READ },
		{ "WriteCopy", PAGE_WRITECOPY },
		{ "Execute/Read/Write", PAGE_EXECUTE_READWRITE },
		{ "Execute/WriteCopy", PAGE_EXECUTE_WRITECOPY },
		{ "No Access", PAGE_NOACCESS },
		{ "Read", PAGE_READONLY },
		{ "Read/Write", PAGE_READWRITE },
	};

	std::string text = std::find_if(std::begin(prot), std::end(prot), [protection](auto& p) {
		return p.Value == (protection & 0xff);
		})->Text;

	static const struct {
		PCSTR Text;
		DWORD Value;
	} extra[] = {
		{ "Guard", PAGE_GUARD },
		{ "No Cache", PAGE_NOCACHE },
		{ "Write Combine", PAGE_WRITECOMBINE },
		{ "Targets Invalid", PAGE_TARGETS_INVALID },
		{ "Targets No Update", PAGE_TARGETS_NO_UPDATE },
	};

	std::for_each(std::begin(extra), std::end(extra), [&text, protection](auto& p) {
		if (p.Value & protection)
			((text += "/") += p.Text);
		});

	return text;
}

void DisplayHeaders() {
	int len = printf(" %-22s %-10s %-8s   %-17s %-17s %-10s %s\n",
		"Base Address", "Size", "State", "Protection", "Alloc. Protection", "Type", "Details");
	printf("%s\n", std::string(len - 1, '-').c_str());
}

std::string GetDetails(HANDLE hProcess, MEMORY_BASIC_INFORMATION& mbi) {
	if (mbi.State != MEM_COMMIT)
		return "";

	if (mbi.Type == MEM_IMAGE || mbi.Type ==  MEM_MAPPED) {
		char path[MAX_PATH];
		if (::GetMappedFileNameA(hProcess, mbi.BaseAddress, path, sizeof(path)) > 0)
			return path;
	}
	return "";
}

std::string AttributesToString(PSAPI_WORKING_SET_EX_BLOCK attributes) {
	if (!attributes.Valid)
		return "(Not in working set)";

	//auto text = "Protection: " + ProtectionToString(attributes.Win32Protection) + " ";
	std::string text;
	if (attributes.Shared)
		text += "Shareable, ";
	else
		text += "Private, ";

	if(attributes.ShareCount > 1)
		text += "Shared, ";

	if (attributes.Locked)
		text += "Locked, ";
	if (attributes.LargePage)
		text += "Large Page, ";
	if (attributes.Bad)
		text += "Bad, ";
	return text.substr(0, text.size() - 2);
}

void DisplayWorkingSetDetails(HANDLE hProcess, MEMORY_BASIC_INFORMATION& mbi) {
	auto pages = mbi.RegionSize >> 12;
	PSAPI_WORKING_SET_EX_INFORMATION info;
	ULONG attributes = 0;
	void* address = nullptr;
	SIZE_T size = 0;
	for (decltype(pages) i = 0; i < pages; i++) {
		info.VirtualAddress = (BYTE*)mbi.BaseAddress + (i << 12);

		if (!::QueryWorkingSetEx(hProcess, &info, sizeof(PSAPI_WORKING_SET_EX_INFORMATION))) {
			printf("  <<<Unable to get working set information>>>\n");
			break;
		}

		if (attributes == 0) {
			address = info.VirtualAddress;
			attributes = (ULONG)info.VirtualAttributes.Flags;
			size = 1 << 12;
		}
		else if(attributes == (ULONG)info.VirtualAttributes.Flags) {
			size += 1 << 12;
		}
		if(attributes != (ULONG)info.VirtualAttributes.Flags || i == pages - 1) {
			printf("  Address: %16p (%10llu KB) Attributes: %08X %s\n",
				address, size >> 10, attributes, AttributesToString(*(PSAPI_WORKING_SET_EX_BLOCK*)&attributes).c_str());
			size = 1 << 12;
			attributes = (ULONG)info.VirtualAttributes.Flags;
			address = info.VirtualAddress;
		}
	}
}

void DisplayBlock(HANDLE hProcess, MEMORY_BASIC_INFORMATION& mbi) {
	printf("%s", mbi.AllocationBase == mbi.BaseAddress ? "*" : " ");
	printf("0x%16p", mbi.BaseAddress);
	printf(" %11llu KB", mbi.RegionSize >> 10);
	printf(" %-10s", StateToString(mbi.State));
	printf(" %-17s", mbi.State != MEM_COMMIT ? "" : ProtectionToString(mbi.Protect).c_str());
	printf(" %-17s", mbi.State == MEM_FREE ? "" : ProtectionToString(mbi.AllocationProtect).c_str());
	printf(" %-8s", mbi.State == MEM_COMMIT ? MemoryTypeToString(mbi.Type) : "");
	printf(" %s\n", GetDetails(hProcess, mbi).c_str());
	if (mbi.State == MEM_COMMIT)
		DisplayWorkingSetDetails(hProcess, mbi);
}

void ShowMemoryMap(HANDLE hProcess) {
	BYTE* address = nullptr;

	MEMORY_BASIC_INFORMATION mbi;

	DisplayHeaders();

	for (;;) {
		if (0 == ::VirtualQueryEx(hProcess, address, &mbi, sizeof(mbi)))
			break;

		DisplayBlock(hProcess, mbi);
		address += mbi.RegionSize;
	}
}

int main(int argc, const char* argv[]) {
	DWORD pid;
	if (argc == 1) {
		printf("No PID specified, using current process...\n");
		pid = ::GetCurrentProcessId();
	}
	else {
		pid = atoi(argv[1]);
	}

	// open process
	HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (!hProcess)
		return Error("Failed to open process");

	printf("Memory map for process %d (0x%X)\n\n", pid, pid);

	ShowMemoryMap(hProcess);

	::CloseHandle(hProcess);
	return 0;
}

