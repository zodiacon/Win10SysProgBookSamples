// sd.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <sddl.h>
#include <string>
#include <AclAPI.h>

std::wstring SidToString(const PSID sid) {
	PWSTR ssid;
	std::wstring result;
	if (::ConvertSidToStringSid(sid, &ssid)) {
		result = ssid;
		::LocalFree(ssid);
	}
	return result;
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

std::string SDControlToString(SECURITY_DESCRIPTOR_CONTROL control) {
	std::string result;
	static const struct {
		DWORD flag;
		PCSTR text;
	} attributes[] = {
		{ SE_OWNER_DEFAULTED, "Owner Defaulted" },
		{ SE_GROUP_DEFAULTED, "Group Defaulted" },
		{ SE_DACL_PRESENT, "DACL Present" },
		{ SE_DACL_DEFAULTED, "DACL Defaulted" },
		{ SE_SACL_PRESENT, "SACL Present" },
		{ SE_SACL_DEFAULTED, "SACL Defaulted" },
		{ SE_DACL_AUTO_INHERIT_REQ, "DACL Auto Inherit Required" },
		{ SE_SACL_AUTO_INHERIT_REQ, "SACL Auto Inherit Required" },
		{ SE_DACL_AUTO_INHERITED, "DACL Auto Inherited" },
		{ SE_SACL_AUTO_INHERITED, "SACL Auto Inherited" },
		{ SE_DACL_PROTECTED, "DACL Protected" },
		{ SE_SACL_PROTECTED, "SACL Protected" },
		{ SE_RM_CONTROL_VALID, "RM Control Valid" },
		{ SE_SELF_RELATIVE, "Self Relative" },
	};

	for (const auto& attr : attributes)
		if ((attr.flag & control) == attr.flag)
			(result += attr.text) += ", ";

	if (!result.empty())
		result = result.substr(0, result.size() - 2);
	else
		result = "(none)";
	return result;
}

const char* AceTypeToString(BYTE type) {
	switch (type) {
		case ACCESS_ALLOWED_ACE_TYPE: return "ALLOW";
		case ACCESS_DENIED_ACE_TYPE: return "DENY";
		case ACCESS_ALLOWED_CALLBACK_ACE_TYPE: return "ALLOW CALLBACK";
		case ACCESS_DENIED_CALLBACK_ACE_TYPE: return "DENY CALLBACK";
		case ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE: return "ALLOW CALLBACK OBJECT";
		case ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE: return "DENY CALLBACK OBJECT";
	}
	return "<unknown>";
}

void DisplayAce(PACE_HEADER header, int index) {
	printf("ACE %2d: Size: %2d bytes, Flags: 0x%02X Type: %s\n", 
		index, header->AceSize, header->AceFlags, AceTypeToString(header->AceType));
	switch (header->AceType) {
		case ACCESS_ALLOWED_ACE_TYPE:
		case ACCESS_DENIED_ACE_TYPE:	// have the same binary layout
		{
			auto data = (ACCESS_ALLOWED_ACE*)header;
			printf("\tAccess: 0x%08X %ws (%ws)\n", data->Mask,
				GetUserNameFromSid((PSID)&data->SidStart).c_str(), SidToString((PSID)&data->SidStart).c_str());
		}
		break;
	}
}

void DisplaySD(const PSECURITY_DESCRIPTOR sd) {
	auto len = ::GetSecurityDescriptorLength(sd);
	printf("SD Length: %u bytes\n", len);

	PWSTR sddl;
	if (::ConvertSecurityDescriptorToStringSecurityDescriptor(sd, SDDL_REVISION_1,
		OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
		&sddl, nullptr) && sddl) {
		printf("SD: %ws\n", sddl);
		::LocalFree(sddl);
	}

	SECURITY_DESCRIPTOR_CONTROL control;
	DWORD revision;
	if (::GetSecurityDescriptorControl(sd, &control, &revision)) {
		printf("Control: %s\n", SDControlToString(control).c_str());
	}
	PSID sid;
	BOOL defaulted;
	if (::GetSecurityDescriptorOwner(sd, &sid, &defaulted)) {
		if (sid)
			printf("Owner: %ws (%ws)\n", GetUserNameFromSid(sid).c_str(), SidToString(sid).c_str());
		else
			printf("No owner\n");
	}

	BOOL present;
	PACL dacl;
	if (::GetSecurityDescriptorDacl(sd, &present, &dacl, &defaulted)) {
		if (!present)
			printf("NULL DACL - object is unprotected\n");
		else {
			printf("DACL: ACE count: %d\n", (int)dacl->AceCount);
			PACE_HEADER header;
			for (int i = 0; i < dacl->AceCount; i++) {
				::GetAce(dacl, i, (PVOID*)&header);
				DisplayAce(header, i);
			}
		}
	}
}

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2) {
		printf("Usage: sd [[-p <pid>] | [-t <tid>] | [-f <filename>] | [-k <regkey>] | [objectname] [-s sddl]]\n");
		printf("If no arguments specified, shows the current process security descriptor\n");
	}

	bool thisProcess = argc == 1;
	HANDLE hObject = argc == 1 ? ::GetCurrentProcess() : nullptr;
	SE_OBJECT_TYPE type = SE_UNKNOWN_OBJECT_TYPE;
	PCWSTR name = nullptr;
	bool sddl = false;

	if(argc > 2) {
		name = argv[2];
		if (::_wcsicmp(argv[1], L"-p") == 0)
			hObject = ::OpenProcess(READ_CONTROL, FALSE, _wtoi(argv[2]));
		else if (::_wcsicmp(argv[1], L"-t") == 0)
			hObject = ::OpenThread(READ_CONTROL, FALSE, _wtoi(argv[2]));
		else if (::_wcsicmp(argv[1], L"-f") == 0)
			type = SE_FILE_OBJECT;
		else if (::_wcsicmp(argv[1], L"-k") == 0)
			type = SE_REGISTRY_KEY;
		else if (::_wcsicmp(argv[1], L"-s") == 0)
			sddl = true;
	}
	else if (argc == 2) {
		name = argv[1];
		type = SE_KERNEL_OBJECT;
	}

	PSECURITY_DESCRIPTOR sd = nullptr;
	if (sddl) {
		if (!::ConvertStringSecurityDescriptorToSecurityDescriptor(name, SDDL_REVISION_1, &sd, nullptr)) {
			printf("Error: %u\n", ::GetLastError());
			return 1;
		}
		DisplaySD(sd);
		::LocalFree(sd);
		return 0;
	}
	else if (!hObject && type == SE_UNKNOWN_OBJECT_TYPE) {
		printf("Error: %u\n", ::GetLastError());
		return 1;
	}

	BYTE buffer[1 << 12];
	DWORD error = 0;
	if (!hObject) {
		error = ::GetNamedSecurityInfo(name, type, OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
			nullptr, nullptr, nullptr, nullptr, &sd);
		if(error == ERROR_SUCCESS) {
			DisplaySD(sd);
			::LocalFree(sd);
		}
	}
	else {
		DWORD len;
		auto success = ::GetKernelObjectSecurity(hObject,
			OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
			(PSECURITY_DESCRIPTOR)buffer, sizeof(buffer), &len);
		if (success)
			DisplaySD((PSECURITY_DESCRIPTOR)buffer);
		else
			error = ::GetLastError();
	}
	if(error)
		printf("Error getting Security Descriptor (%u)\n", error);

	if (hObject && !thisProcess)
		::CloseHandle(hObject);

	return 0;
}
