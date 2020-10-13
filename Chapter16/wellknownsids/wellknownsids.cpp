// wellknownsids.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <sddl.h>
#include <stdio.h>
#include <NTSecAPI.h>

const char* SidNameUseToString(SID_NAME_USE use) {
	switch (use) {
		case SidTypeUser: return "User";
		case SidTypeGroup: return "Group";
		case SidTypeDomain: return "Domain";
		case SidTypeAlias: return "Alias";
		case SidTypeWellKnownGroup: return "Well Known Group";
		case SidTypeDeletedAccount: return "Deleted Account";
		case SidTypeInvalid: return "Invalid";
		case SidTypeUnknown: return "Unknown";
		case SidTypeComputer: return "Computer";
		case SidTypeLabel: return "Label";
		case SidTypeLogonSession: return "Logon Session";
	}
	return "";
}

int main() {
	BYTE buffer[SECURITY_MAX_SID_SIZE];
	PWSTR name;

	WCHAR accountName[64] = { 0 }, domainName[64] = { 0 };
	SID_NAME_USE use;

	for (int i = 0; i < 120; i++) {
		DWORD size = sizeof(buffer);
		if (!::CreateWellKnownSid((WELL_KNOWN_SID_TYPE)i, nullptr, (PSID)buffer, &size))
			continue;

		::ConvertSidToStringSid((PSID)buffer, &name);
		DWORD accountNameSize = _countof(accountName);
		DWORD domainNameSize = _countof(domainName);
		::LookupAccountSid(nullptr, (PSID)buffer, accountName, &accountNameSize,
			domainName, &domainNameSize, &use);

		printf("Well known sid %3d: %-20ws %ws\\%ws (%s)\n", i, 
			name, domainName, accountName, SidNameUseToString(use));
		::LocalFree(name);
	}

	return 0;
}

