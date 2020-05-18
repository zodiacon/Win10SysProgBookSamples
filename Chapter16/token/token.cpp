// token.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <string>
#include <sddl.h>

void DisplayTokenInfo(HANDLE hToken);

int main(int argc, const char* argv[]) {
	DWORD id = argc > 1 ? atoi(argv[1]) : 0;

	if (id == 0 && argc > 1) {
		printf("Usage: token <pid or tid>\n");
		return 1;
	}

	HANDLE hObject = nullptr;
	HANDLE hToken = nullptr;
	while (id) {
		hObject = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, id);
		if (hObject) {
			printf("Opening token for process %u\n", id);
			::OpenProcessToken(hObject, TOKEN_QUERY, &hToken);
		}
		else if (::GetLastError() != ERROR_ACCESS_DENIED) {
			hObject = ::OpenThread(THREAD_QUERY_INFORMATION, FALSE, id);
			if (hObject) {
				printf("Opening token for thread %u\n", id);
				if (!::OpenThreadToken(hObject, TOKEN_QUERY, FALSE, &hToken)) {
					printf("Thread has no impersonation token. Defaulting to its process\n");
					id = ::GetProcessIdOfThread(hObject);
					::CloseHandle(hObject);
					continue;
				}
			}
		}
		break;
	}
	if (id == 0) {
		printf("Opening current process token\n");
		::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken);
	}
	if (hObject)
		::CloseHandle(hObject);

	if (!hToken) {
		printf("Failed to open token (%u)\n", ::GetLastError());
	}
	else {
		printf("\n");
		DisplayTokenInfo(hToken);
		::CloseHandle(hToken);
	}
	return 0;
}

ULONGLONG LuidToNum(const LUID& luid) {
	return *(const ULONGLONG*)&luid;
}

const char* ImpersonationLevelToString(SECURITY_IMPERSONATION_LEVEL level) {
	switch (level) {
		case SecurityAnonymous: return "Anonymous";
		case SecurityIdentification: return "Identification";
		case SecurityImpersonation: return "Impersonation";
		case SecurityDelegation: return "Delegation";
	}
	return "Unknown";
}

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


std::wstring GetUserNameFromSid(const PSID sid, SID_NAME_USE* puse = nullptr) {
	WCHAR name[64], domain[64];
	DWORD lname = _countof(name), ldomain = _countof(domain);
	SID_NAME_USE use;
	if (::LookupAccountSid(nullptr, sid, name, &lname, domain, &ldomain, &use)) {
		if (puse)
			*puse = use;
		return std::wstring(domain) + L"\\" + name;
	}
	return L"";
}

std::string SidAttributesToString(DWORD sidAttributes) {
	std::string result;
	static struct {
		DWORD flag;
		PCSTR text;
	} attributes[] = {
		{ SE_GROUP_ENABLED, "Enabled" },
		{ SE_GROUP_ENABLED_BY_DEFAULT, "Default Enabled" },
		{ SE_GROUP_INTEGRITY, "Integrity" },
		{ SE_GROUP_INTEGRITY_ENABLED, "Integrity Enabled" },
		{ SE_GROUP_LOGON_ID, "Logon ID" },
		{ SE_GROUP_MANDATORY, "Mandatory" },
		{ SE_GROUP_OWNER, "Owner" },
		{ SE_GROUP_RESOURCE, "Domain Local" },
		{ SE_GROUP_USE_FOR_DENY_ONLY, "Deny Only" },
	};

	for (const auto& attr : attributes)
		if ((attr.flag & sidAttributes) == attr.flag)
			(result += attr.text) += ", ";

	if (!result.empty())
		result = result.substr(0, result.size() - 2);
	else
		result = "(none)";
	return result;
}

std::wstring SidToString(const PSID sid) {
	PWSTR ssid;
	std::wstring result;
	if (::ConvertSidToStringSid(sid, &ssid)) {
		result = ssid;
		::LocalFree(ssid);
	}
	return result;
}

void DisplaySidsAndAttributes(PSID_AND_ATTRIBUTES saas, DWORD count) {
	for (DWORD i = 0; i < count; i++) {
		const auto& saa = saas[i];
		printf("SID: %ws\n", SidToString(saa.Sid).c_str());
		SID_NAME_USE use = SidTypeUnknown;
		printf("Name: %ws", GetUserNameFromSid(saa.Sid, &use).c_str());
		printf(" (%s)\n", SidNameUseToString(use));
		printf("%s\n\n", SidAttributesToString(saa.Attributes).c_str());
	}
}

std::string PrivilegeAttributesToString(DWORD pattributes) {
	std::string result;
	static struct {
		DWORD flag;
		PCSTR text;
	} attributes[] = {
		{ SE_PRIVILEGE_ENABLED, "Enabled" },
		{ SE_PRIVILEGE_ENABLED_BY_DEFAULT, "Default Enabled" },
	};

	for (const auto& attr : attributes)
		if ((attr.flag & pattributes) == attr.flag)
			(result += attr.text) += ", ";

	if (!result.empty())
		result = result.substr(0, result.size() - 2);
	else
		result = "Disabled";
	return result;
}

void DisplayPrivileges(PLUID_AND_ATTRIBUTES privs, DWORD count) {
	WCHAR name[64];
	for (DWORD i = 0; i < count; i++) {
		auto& priv = privs[i];
		DWORD len = _countof(name);
		if (::LookupPrivilegeName(nullptr, &priv.Luid, name, &len)) {
			printf("%-41ws (%s)\n", name, PrivilegeAttributesToString(priv.Attributes).c_str());
		}
	}
}

void DisplayTokenInfo(HANDLE hToken) {
	DWORD len;
	BYTE buffer[1 << 12];
	TOKEN_STATISTICS stats;

	if (::GetTokenInformation(hToken, TokenUser, buffer, sizeof(buffer), &len)) {
		auto data = (TOKEN_USER*)buffer;
		printf("User SID: %ws\n", SidToString(data->User.Sid).c_str());
		printf("Username: %ws\n\n", GetUserNameFromSid(data->User.Sid).c_str());
	}

	if (::GetTokenInformation(hToken, TokenStatistics, &stats, sizeof(stats), &len)) {
		printf("Token ID: 0x%08llX\n", LuidToNum(stats.TokenId));
		printf("Logon Session ID: 0x%08llX\n", LuidToNum(stats.AuthenticationId));
		printf("Token Type: %s\n", stats.TokenType == TokenPrimary ? "Primary" : "Impersonation");
		if (stats.TokenType == TokenImpersonation)
			printf("Impersonation level: %s\n", ImpersonationLevelToString(stats.ImpersonationLevel));

		printf("Dynamic charged (bytes): %lu\n", stats.DynamicCharged);
		printf("Dynamic available (bytes): %lu\n", stats.DynamicAvailable);
		printf("Group count: %lu\n", stats.GroupCount);
		printf("Privilege count: %lu\n", stats.PrivilegeCount);
		printf("Modified ID: %08llX\n\n", LuidToNum(stats.ModifiedId));
	}

	if (::GetTokenInformation(hToken, TokenOwner, buffer, sizeof(buffer), &len)) {
		auto owner = (TOKEN_OWNER*)buffer;
		printf("Default owner SID: %ws\n", SidToString(owner->Owner).c_str());
		printf("Default owner: %ws\n\n", GetUserNameFromSid(owner->Owner).c_str());
	}

	if (::GetTokenInformation(hToken, TokenGroupsAndPrivileges, buffer, sizeof(buffer), &len)) {
		int count = printf("Groups\n");
		printf("%s\n", std::string(count - 1, '-').c_str());
		auto data = (TOKEN_GROUPS_AND_PRIVILEGES*)buffer;
		printf("SID count: %u\n\n", data->SidCount);
		DisplaySidsAndAttributes(data->Sids, data->SidCount);

		count = printf("Privileges\n");
		printf("%s\n", std::string(count - 1, '-').c_str());
		DisplayPrivileges(data->Privileges, data->PrivilegeCount);
	}

	if (::GetTokenInformation(hToken, TokenCapabilities, buffer, sizeof(buffer), &len)) {
		auto caps = (TOKEN_GROUPS*)buffer;
		if (caps->GroupCount > 0) {
			int count = printf("Capabilities\n");
			printf("%s\n", std::string(count - 1, '-').c_str());
			DisplaySidsAndAttributes(caps->Groups, caps->GroupCount);
		}
	}
}
