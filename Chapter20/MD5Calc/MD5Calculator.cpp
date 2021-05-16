#include "pch.h"
#include "MD5Calculator.h"

TRACELOGGING_DECLARE_PROVIDER(g_Provider);

std::vector<uint8_t> MD5Calculator::Calculate(PCWSTR path) {
	TraceLoggingWrite(g_Provider, "CalculatingMD5",
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingValue(path, "Path"));

	std::vector<uint8_t> md5;

	wil::unique_hfile hFile(::CreateFile(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr));
	if (!hFile)
		return md5;

	wil::unique_handle hMemMap(::CreateFileMapping(hFile.get(), nullptr, PAGE_READONLY, 0, 0, nullptr));
	if (!hMemMap)
		return md5;

	wil::unique_hcryptprov hProvider;
	if (!::CryptAcquireContext(hProvider.addressof(), nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		return md5;

	wil::unique_hcrypthash hHash;
	if(!::CryptCreateHash(hProvider.get(), CALG_MD5, 0, 0, hHash.addressof()))
		return md5;

	wil::unique_mapview_ptr<BYTE> buffer((BYTE*)::MapViewOfFile(hMemMap.get(), FILE_MAP_READ, 0, 0, 0));
	if (!buffer)
		return md5;

	auto size = ::GetFileSize(hFile.get(), nullptr);
	TraceLoggingWrite(g_Provider, "CalculatingMD5Begin",
		TraceLoggingLevel(TRACE_LEVEL_VERBOSE),
		TraceLoggingOpcode(EVENT_TRACE_TYPE_START),
		TraceLoggingValue(size, "FileSize"));

	if (!::CryptHashData(hHash.get(), buffer.get(), size, 0))
		return md5;

	DWORD hashSize;
	DWORD len = sizeof(DWORD);
	if (!::CryptGetHashParam(hHash.get(), HP_HASHSIZE, (BYTE*)&hashSize, &len, 0))
		return md5;

	md5.resize(len = hashSize);
	::CryptGetHashParam(hHash.get(), HP_HASHVAL, md5.data(), &len, 0);

	TraceLoggingWrite(g_Provider, "CalculatingMD5End",
		TraceLoggingLevel(TRACE_LEVEL_VERBOSE),
		TraceLoggingOpcode(EVENT_TRACE_TYPE_END),
		TraceLoggingBinary(md5.data(), len, "MD5", "MD5 hash result"));

	return md5;
}
