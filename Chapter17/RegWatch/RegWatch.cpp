// RegWatch.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2) {
		printf("Usage: RegWatch [options] <key>\n");
		printf("\tOptions: -r (recursive)\n");
		return 0;
	}

	bool recurse = false;

	for (int i = 1; i < argc - 1; i++) {
		if (::_wcsicmp(argv[i], L"-r") == 0)
			recurse = true;
	}

	HKEY root = nullptr;

	auto key = argv[argc - 1];
	if (::_wcsnicmp(key, L"HKCR", 4) == 0)
		root = HKEY_CLASSES_ROOT;
	else if (::_wcsnicmp(key, L"HKLM", 4) == 0)
		root = HKEY_LOCAL_MACHINE;
	else if (::_wcsnicmp(key, L"HKCU", 4) == 0)
		root = HKEY_CURRENT_USER;
	else if (::_wcsnicmp(key, L"HKU", 3) == 0)
		root = HKEY_USERS;
	else {
		printf("Illegal root key.\n");
		return 1;
	}

	auto bs = ::wcschr(key, L'\\');
	auto path = bs ? (bs + 1) : nullptr;

	HKEY hKey;
	auto error = ::RegOpenKeyEx(root, path, 0, KEY_NOTIFY, &hKey);
	if (error != ERROR_SUCCESS) {
		printf("Failed to open key (%u)\n", error);
		return 1;
	}

	DWORD notifyFlags = REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET;

	printf("Watching...\n");
	while (ERROR_SUCCESS == ::RegNotifyChangeKeyValue(hKey, recurse, notifyFlags, nullptr, FALSE)) {
		printf("Changed occurred.\n");
	}
	::RegCloseKey(hKey);

	return 0;
}

