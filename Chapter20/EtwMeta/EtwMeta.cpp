// EtwMeta.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <tdh.h>
#include <string>
#include <stdio.h>
#include <memory>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <VersionHelpers.h>

#pragma comment(lib, "tdh")

struct EtwProvider {
	std::wstring Name;
	GUID Guid;
};

std::vector<EtwProvider> EnumProviders() {
	std::vector<EtwProvider> providers;

	// first call with NULL and zero
	ULONG size = 0;
	auto error = ::TdhEnumerateProviders(nullptr, &size);
	assert(error == ERROR_INSUFFICIENT_BUFFER);

	// allocate with the required size
	auto buffer = std::make_unique<BYTE[]>(size);
	if(!buffer)
		return providers;

	auto data = reinterpret_cast<PROVIDER_ENUMERATION_INFO*>(buffer.get());
	// second call
	error = ::TdhEnumerateProviders(data, &size);
	assert(error == ERROR_SUCCESS);
	if(error != ERROR_SUCCESS)
		return providers;

	// build a vector of providers
	providers.reserve(data->NumberOfProviders);
	for(ULONG i = 0; i < data->NumberOfProviders; i++) {
		const auto& item = data->TraceProviderInfoArray[i];
		EtwProvider provider;
		provider.Name.assign((PCWSTR)(buffer.get() + item.ProviderNameOffset));
		provider.Guid = item.ProviderGuid;
		providers.push_back(std::move(provider));
	}

	// sort vector by name
	std::sort(providers.begin(), providers.end(), [](const auto& p1, const auto& p2) {
		return ::_wcsicmp(p1.Name.c_str(), p2.Name.c_str()) < 0;
		});

	return providers;
}

void DumpEventDescriptor(const EVENT_DESCRIPTOR& desc) {
	printf("Id: %u (0x%X) Version: %u Channel: %u Level: %u Opcode: %u Task: %u Keyword: 0x%llX\n",
		desc.Id, desc.Id, desc.Version, desc.Channel, desc.Level, desc.Opcode, desc.Task, desc.Keyword);
}

std::string InTypeToString(USHORT type) {
	switch(type) {
		case TDH_INTYPE_BOOLEAN: return "Boolean";
		case TDH_INTYPE_DOUBLE: return "Double";
		case TDH_INTYPE_ANSISTRING: return "Ansi String";
		case TDH_INTYPE_UNICODESTRING: return "Unicode String";
		case TDH_INTYPE_INT16: return "Int16";
		case TDH_INTYPE_INT32: return "Int32";
		case TDH_INTYPE_INT64: return "Int64";
		case TDH_INTYPE_INT8: return "Int8";
		case TDH_INTYPE_BINARY: return "Binary";
		case TDH_INTYPE_ANSICHAR: return "Ansi Char";
		case TDH_INTYPE_UNICODECHAR: return "Unicode Char";
		case TDH_INTYPE_FILETIME: return "FILETIME";
		case TDH_INTYPE_SYSTEMTIME: return "SYSTEMTIME";
		case TDH_INTYPE_COUNTEDSTRING: return "Counted String";
		case TDH_INTYPE_COUNTEDANSISTRING: return "Counted Ansi String";
		case TDH_INTYPE_FLOAT: return "Float";
		case TDH_INTYPE_GUID: return "GUID";
		case TDH_INTYPE_HEXINT32: return "Hex Int32";
		case TDH_INTYPE_HEXINT64: return "Hex Int64";
		case TDH_INTYPE_POINTER: return "Pointer";
		case TDH_INTYPE_SID: return "SID";
		case TDH_INTYPE_SIZET: return "SIZE_T";
		case TDH_INTYPE_UINT16: return "UInt16";
		case TDH_INTYPE_UINT32: return "UInt32";
		case TDH_INTYPE_UINT64: return "UInt64";
		case TDH_INTYPE_UINT8: return "UInt8";
	}
	return std::to_string(type);
}

std::string OutTypeToString(USHORT type) {
	switch(type) {
		case TDH_OUTTYPE_BOOLEAN: return "Boolean";
		case TDH_OUTTYPE_DOUBLE: return "Double";
		case TDH_OUTTYPE_NULL: return "NULL";
		case TDH_OUTTYPE_INT: return "Int";
		case TDH_OUTTYPE_SHORT: return "Short";
		case TDH_OUTTYPE_BYTE: return "Byte";
		case TDH_OUTTYPE_UNSIGNEDBYTE: return "UByte";
		case TDH_OUTTYPE_UNSIGNEDINT: return "UInt";
		case TDH_OUTTYPE_UNSIGNEDLONG: return "ULong";
		case TDH_OUTTYPE_LONG: return "Long";
		case TDH_OUTTYPE_DATETIME: return "DateTime";
		case TDH_OUTTYPE_DATETIME_UTC: return "DateTime UTC";
		case TDH_OUTTYPE_CODE_POINTER: return "Code Pointer";
		case TDH_OUTTYPE_ERRORCODE: return "Error Code";
		case TDH_OUTTYPE_ETWTIME: return "ETW Time";
		case TDH_OUTTYPE_FLOAT: return "Float";
		case TDH_OUTTYPE_GUID: return "GUID";
		case TDH_OUTTYPE_HEXINT32: return "Hex Int32";
		case TDH_OUTTYPE_HEXINT64: return "Hex Int64";
		case TDH_OUTTYPE_HEXBINARY: return "Hex Binary";
		case TDH_OUTTYPE_CULTURE_INSENSITIVE_DATETIME: return "CI DateTime";
		case TDH_OUTTYPE_HEXINT16: return "Hex Int16";
		case TDH_OUTTYPE_HRESULT: return "HRESULT";
		case TDH_OUTTYPE_IPV4: return "IPv4";
		case TDH_OUTTYPE_IPV6: return "IPv6";
		case TDH_OUTTYPE_JSON: return "JSON";
		case TDH_OUTTYPE_NTSTATUS: return "NTSTATUS";
		case TDH_OUTTYPE_NOPRINT: return "No Print";
		case TDH_OUTTYPE_PID: return "PID";
		case TDH_OUTTYPE_STRING: return "String";
		case TDH_OUTTYPE_SOCKETADDRESS: return "Socket Address";
		case TDH_OUTTYPE_PORT: return "Port";
		case TDH_OUTTYPE_TID: return "TID";
		case TDH_OUTTYPE_UNSIGNEDSHORT: return "UShort";
	}
	return std::to_string(type);
}

std::wstring PropertyFlagsToString(ULONG flags) {
	static const struct {
		PCWSTR text;
		ULONG flag;
	} data[] = {
		{ L"Struct", PropertyStruct },
		{ L"Param Length", PropertyParamLength },
		{ L"Param Count", PropertyParamCount },
		{ L"XML Fragment", PropertyWBEMXmlFragment } ,
		{ L"Param Fixed Length", PropertyParamFixedLength },
		{ L"Param Fixed Count", PropertyParamFixedCount },
		{ L"Has Tags", PropertyHasTags },
		{ L"Has Custom Schema", PropertyHasCustomSchema },
	};

	std::wstring result;
	for(auto& d : data) {
		if((d.flag & flags) == d.flag)
			(result += d.text) += L", ";
	}
	if(result.empty())
		return L"None";
	return result.substr(0, result.size() - 2);
}

void DumpPropertyInfo(const BYTE* buffer, const EVENT_PROPERTY_INFO& info) {
	printf("\tProperty name: %ws (Flags: %ws)\n", (PCWSTR)(buffer + info.NameOffset), PropertyFlagsToString(info.Flags).c_str());
	if((info.Flags & PropertyStruct) == 0) {
		auto inType = info.nonStructType.InType;
		auto outType = info.nonStructType.OutType;
		std::wstring mapName;
		if(info.nonStructType.MapNameOffset)
			mapName = (PCWSTR)(buffer + info.nonStructType.MapNameOffset);
		printf("\t\tIn: %s Out: %s%ws\n", InTypeToString(inType).c_str(), OutTypeToString(outType).c_str(),
			mapName.empty() ? L"" : (L" (" + mapName + L")").c_str());
	}
}

bool DumpEventInfo(const GUID& guid, const EVENT_DESCRIPTOR& desc) {
	DumpEventDescriptor(desc);

	ULONG size = 0;
	auto error = ::TdhGetManifestEventInformation((LPGUID)&guid, (PEVENT_DESCRIPTOR)&desc, nullptr, &size);
	if(error != ERROR_INSUFFICIENT_BUFFER)
		return false;

	auto buffer = std::make_unique<BYTE[]>(size);
	if(!buffer)
		return false;

	auto data = reinterpret_cast<TRACE_EVENT_INFO*>(buffer.get());
	error = ::TdhGetManifestEventInformation((LPGUID)&guid, (PEVENT_DESCRIPTOR)&desc, data, &size);
	if(ERROR_SUCCESS != error)
		return false;

	if(data->EventNameOffset)
		printf("Event Name: %ws\n", (PCWSTR)(buffer.get() + data->EventNameOffset));
	if(data->KeywordsNameOffset)
		printf("Keyword: %ws\n", (PCWSTR)(buffer.get() + data->KeywordsNameOffset));
	if(data->TaskNameOffset)
		printf("Task: %ws\n", (PCWSTR)(buffer.get() + data->TaskNameOffset));
	if(data->ChannelNameOffset)
		printf("Channel: %ws\n", (PCWSTR)(buffer.get() + data->ChannelNameOffset));
	if(data->LevelNameOffset)
		printf("Level: %ws\n", (PCWSTR)(buffer.get() + data->LevelNameOffset));
	if(data->OpcodeNameOffset)
		printf("Opcode: %ws\n", (PCWSTR)(buffer.get() + data->OpcodeNameOffset));
	if(data->ProviderMessageOffset)
		printf("Provider Message: %ws\n", (PCWSTR)(buffer.get() + data->ProviderMessageOffset));
	if(data->EventMessageOffset)
		printf("Event Message:\n%ws\n", (PCWSTR)(buffer.get() + data->EventMessageOffset));

	printf("Property Count: %u\n", data->PropertyCount);
	for(ULONG i = 0; i < data->TopLevelPropertyCount; i++) {
		auto& prop = data->EventPropertyInfoArray[i];
		DumpPropertyInfo(buffer.get(), prop);
	}
	printf("\n");
	return true;
}

bool DumpProviderEvents(const GUID& guid) {
	ULONG size = 0;
	auto error = ::TdhEnumerateManifestProviderEvents((LPGUID)&guid, nullptr, &size);
	if(error != ERROR_INSUFFICIENT_BUFFER)
		return false;

	auto buffer = std::make_unique<BYTE[]>(size);
	if(!buffer)
		return false;

	auto data = reinterpret_cast<PROVIDER_EVENT_INFO*>(buffer.get());
	error = ::TdhEnumerateManifestProviderEvents((LPGUID)&guid, data, &size);
	if(error != ERROR_SUCCESS)
		return false;

	printf("Events: %u\n", data->NumberOfEvents);
	for(ULONG i = 0; i < data->NumberOfEvents; i++) {
		auto& info = data->EventDescriptorsArray[i];
		DumpEventInfo(guid, info);
	}
	return true;
}

bool DumpProviderFilters(const GUID& guid) {
	ULONG size = 0;
	ULONG count;
	if(ERROR_INSUFFICIENT_BUFFER != ::TdhEnumerateProviderFilters((LPGUID)&guid, 0, nullptr, &count, nullptr, &size))
		return false;

	auto buffer = std::make_unique<BYTE[]>(size);
	if(!buffer)
		return false;

	auto info = reinterpret_cast<PPROVIDER_FILTER_INFO>(buffer.get());
	if(ERROR_SUCCESS != ::TdhEnumerateProviderFilters((LPGUID)&guid, 0, nullptr, &count, &info, &size))
		return false;

	for(ULONG i = 0; i < count; i++) {
		auto& fi = info[i];
		printf("Filter Id: %d Version: %d Message: %ws Properties: %u \n",
			fi.Id, fi.Version, (PCWSTR)(buffer.get() + fi.MessageOffset), fi.PropertyCount);
	}
	return true;
}

void DisplayProviderFields(const GUID& guid) {
	struct {
		EVENT_FIELD_TYPE type;
		PCWSTR text;
	} fields[] = {
		EventKeywordInformation, L"Keywords",
		EventLevelInformation, L"Levels",
		EventChannelInformation, L"Channels",
		EventTaskInformation, L"Tasks",
		EventOpcodeInformation, L"Opcodes"
	};
	for(auto& field : fields) {
		ULONG size = 0;
		if(ERROR_INSUFFICIENT_BUFFER != 
			::TdhEnumerateProviderFieldInformation((LPGUID)&guid, field.type, nullptr, &size))
			continue;

		auto buffer = std::make_unique<BYTE[]>(size);
		auto info = reinterpret_cast<PPROVIDER_FIELD_INFOARRAY>(buffer.get());
		if(ERROR_SUCCESS != ::TdhEnumerateProviderFieldInformation((LPGUID)&guid, field.type, info, &size))
			continue;

		printf("%ws:\n", field.text);
		for(ULONG i = 0; i < info->NumberOfElements; i++) {
			auto& item = info->FieldInfoArray[i];
			printf("\tValue: 0x%016llX Name: %ws %ws\n", item.Value,
				(PCWSTR)(buffer.get() + item.NameOffset),
				item.DescriptionOffset == 0 ? L"" : (PCWSTR)(buffer.get() + item.DescriptionOffset));
		}
	}
}

bool DisplayProviderInfo(const std::wstring& name) {
	auto providers = EnumProviders();
	WCHAR sguid[64];
	auto findByNameOrGuid = [&](auto& p) {
		if(_wcsicmp(p.Name.c_str(), name.c_str()) == 0)
			return true;
		::StringFromGUID2(p.Guid, sguid, _countof(sguid));
		return _wcsicmp(name.c_str(), sguid) == 0;
	};

	auto it = std::find_if(providers.begin(), providers.end(), findByNameOrGuid);
	if(it == providers.end())
		return false;

	auto& provider = *it;
	printf("Provider Name: %ws\n", (PCWSTR)provider.Name.c_str());
	::StringFromGUID2(provider.Guid, sguid, _countof(sguid));
	printf("Provider GUID: %ws\n", sguid);

	DisplayProviderFields(provider.Guid);

	if(!DumpProviderEvents(provider.Guid))
		printf("No event information provided\n");

	if(::IsWindows7OrGreater())
		DumpProviderFilters(provider.Guid);

	return true;
}


int wmain(int argc, const wchar_t* argv[]) {
	bool listAll = true;
	if(argc > 1) {
		if(!DisplayProviderInfo(argv[1]))
			printf("Unknown provider name/guid\n");

		listAll = false;
	}

	if(listAll) {
		auto providers = EnumProviders();
		// display results
		WCHAR sguid[64];
		for(auto& provider : providers) {
			::StringFromGUID2(provider.Guid, sguid, _countof(sguid));
			printf("%-50ws %ws\n", provider.Name.c_str(), sguid);
		}
		printf("%u Providers.\n", (uint32_t)providers.size());
	}
	return 0;
}

