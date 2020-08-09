// DumpKey.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <memory>
#include <atltime.h>
#include <atlstr.h>

std::pair<CString, CString> GetValueAsString(const BYTE* data, DWORD size, DWORD type) {
	CString value, stype;
	switch (type) {
		case REG_DWORD:
			stype = L"REG_DWORD";
			value.Format(L"%u (0x%X)", *(DWORD*)data, *(DWORD*)data);
			break;

		case REG_QWORD:
			stype = L"REG_QWORD";
			value.Format(L"%llu (0x%llX)", *(DWORD64*)data, *(DWORD64*)data);
			break;

		case REG_SZ:
			stype = L"REG_SZ";
			value = (PCWSTR)data;
			break;

		case REG_EXPAND_SZ:
			stype = L"REG_EXPAND_SZ";
			value = (PCWSTR)data;
			break;

		case REG_BINARY:
			stype = L"REG_BINARY";
			for (DWORD i = 0; i < size; i++)
				value.Format(L"%s%02X ", value, data[i]);
			break;

		default:
			stype.Format(L"%u", type);
			value = L"(Unsupported)";
			break;
	}

	return { stype, value };
}

void DumpKeyValues(HKEY hKey) {
}

void DumpKey(HKEY hKey, bool dumpKeys, bool dumpValues, bool recurse) {
	DWORD nsubkeys, nvalues;
	DWORD maxValueSize;
	DWORD maxValueNameLen;
	FILETIME modified;
	if (ERROR_SUCCESS != ::RegQueryInfoKey(hKey, nullptr, nullptr, nullptr, &nsubkeys,
		nullptr, nullptr, &nvalues, &maxValueNameLen, &maxValueSize, nullptr, &modified))
		return;

	printf("Subkeys: %u Values: %u\n", nsubkeys, nvalues);

	if (dumpValues) {
		DWORD type;
		auto value = std::make_unique<BYTE[]>(maxValueSize);
		auto name = std::make_unique<WCHAR[]>(maxValueNameLen + 1);

		printf("values:\n");
		for (DWORD i = 0; ; i++) {
			DWORD cname = maxValueNameLen + 1;
			DWORD size = maxValueSize;
			auto error = ::RegEnumValue(hKey, i, name.get(), &cname, nullptr, &type, value.get(), &size);
			if (error == ERROR_NO_MORE_ITEMS)
				break;

			auto display = GetValueAsString(value.get(), min(64, size), type);
			printf(" %-30ws %-12ws (%5u B) %ws\n", name.get(), (PCWSTR)display.first, size, (PCWSTR)display.second);
		}
	}

	if (dumpKeys) {
		printf("Keys:\n");
		WCHAR name[256];
		for (DWORD i = 0; ; i++) {
			DWORD cname = _countof(name);
			auto error = ::RegEnumKeyEx(hKey, i, name, &cname, nullptr, nullptr, nullptr, &modified);
			if (error == ERROR_NO_MORE_ITEMS)
				break;

			if (error == ERROR_MORE_DATA) {
				printf(" (Key name too long)\n");
				continue;
			}

			if (error == ERROR_SUCCESS)
				printf(" %-50ws Modified: %ws\n", name, (PCWSTR)CTime(modified).Format(L"%c"));

			if (recurse) {
				HKEY hSubKey;
				if (ERROR_SUCCESS == ::RegOpenKeyEx(hKey, name, 0, KEY_READ, &hSubKey)) {
					printf("--------\n");
					printf("Subkey: %ws\n", name);
					DumpKey(hSubKey, dumpKeys, dumpValues, recurse);
					::RegCloseKey(hSubKey);
				}
			}
		}
	}
}


int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2) {
		printf("Usage:\tDumpKey [options] <key>\n");
		printf("\t<key> can be or start with HKCU, HKCR, HKLM, HKU\n");
		printf("\tOptions: -k (subkeys) -v (values) -r (recurse). If neither -k nor -v is specified, -v is assumed.\n");
		return 0;
	}

	bool keys = false, values = false, recurse = false;

	for (int i = 1; i < argc - 1; i++) {
		if (::_wcsicmp(argv[i], L"-r") == 0)
			recurse = true;
		else if (::_wcsicmp(argv[i], L"-k") == 0)
			keys = true;
		else if (::_wcsicmp(argv[i], L"-v") == 0)
			values = true;
	}

	if (!values && !keys)
		values = true;

	HKEY root = nullptr;
	auto key = argv[argc - 1];
	if(::_wcsnicmp(key, L"HKCR", 4) == 0)
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
	auto error = ::RegOpenKeyEx(root, path, 0, KEY_READ, &hKey);
	if (error != ERROR_SUCCESS) {
		printf("Error opening key: %u\n", error);
		return 1;
	}

	DumpKey(hKey, keys, values, recurse);
	::RegCloseKey(hKey);

	return 0;
}

