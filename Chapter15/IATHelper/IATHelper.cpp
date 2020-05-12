// IATHelper.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "IATHelper.h"
#include <assert.h>

#pragma comment(lib, "DbgHelp")

int IATHelper::HookFunction(PCWSTR callerModule, PCSTR moduleName, PVOID originalProc, PVOID hookProc) {
	HMODULE hMod = ::GetModuleHandle(callerModule);
	if (!hMod)
		return 0;

	ULONG size;
	auto desc = (PIMAGE_IMPORT_DESCRIPTOR)::ImageDirectoryEntryToData(hMod, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &size);
	if (!desc)	// no import table
		return 0;

	int count = 0;
	for (; desc->Name; desc++) {
		auto modName = (PSTR)hMod + desc->Name;
		if (::_stricmp(moduleName, modName) == 0) {
			auto thunk = (PIMAGE_THUNK_DATA)((PBYTE)hMod + desc->FirstThunk);
			for (; thunk->u1.Function; thunk++) {
				auto addr = &thunk->u1.Function;
				if (*(PVOID*)addr == originalProc) {
					// found it
					DWORD old;
					if (::VirtualProtect(addr, sizeof(void*), PAGE_WRITECOPY, &old)) {
						*(void**)addr = (void*)hookProc;
						count++;
					}
				}
			}
			break;
		}
	}
	return count;
}

int IATHelper::HookAllModules(PCSTR moduleName, PVOID originalProc, PVOID hookProc) {
	HMODULE hMod[1024];	// should be enough (famous last words)
	DWORD needed;
	if (!::EnumProcessModules(::GetCurrentProcess(), hMod, sizeof(hMod), &needed))
		return 0;

	assert(needed <= sizeof(hMod));

	WCHAR name[256];
	int count = 0;
	for (DWORD i = 0; i < needed / sizeof(HMODULE); i++) {
		if (::GetModuleBaseName(::GetCurrentProcess(), hMod[i], name, _countof(name))) {
			count += HookFunction(name, moduleName, originalProc, hookProc);
		}
	}

	return count;
}

