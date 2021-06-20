// DateTime.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.globalization.h>
#include <atlbase.h>
#include <roapi.h>
#include <string>
#include <stdio.h>

#pragma comment(lib, "runtimeobject.lib")

using namespace ABI::Windows::Globalization;

HRESULT DisplayInterfaces(IInspectable* pInst);

int main() {
	::RoInitialize(RO_INIT_MULTITHREADED);

	HSTRING hClassName;
	//HRESULT hr = ::WindowsCreateString(
	//	RuntimeClass_Windows_Globalization_Calendar, 
	//	_countof(RuntimeClass_Windows_Globalization_Calendar) - 1, 
	//	&hClassName);
	//ATLASSERT(SUCCEEDED(hr));
	HSTRING_HEADER header;
	HRESULT hr = ::WindowsCreateStringReference(
		RuntimeClass_Windows_Globalization_Calendar, 
		_countof(RuntimeClass_Windows_Globalization_Calendar) - 1, 
		&header, &hClassName);

	CComPtr<IInspectable> spInst;
	hr = ::RoActivateInstance(hClassName, &spInst);

//	::WindowsDeleteString(hClassName);
	if(SUCCEEDED(hr)) {
		DisplayInterfaces(spInst);

		CComQIPtr<ICalendar> spCalendar(spInst);
		if(spCalendar) {
			spCalendar->SetToNow();
			INT32 hour, minute, second;
			spCalendar->get_Hour(&hour);
			spCalendar->get_Minute(&minute);
			spCalendar->get_Second(&second);

			printf("The time is %02d:%02d:%02d\n", hour, minute, second);

			CComQIPtr<ITimeZoneOnCalendar> spTZ(spCalendar);
			if (spTZ) {
				HSTRING hTimeZone;
				if (SUCCEEDED(spTZ->TimeZoneAsFullString(&hTimeZone))) {
					auto tzname = ::WindowsGetStringRawBuffer(hTimeZone, nullptr);	// length is optional
					printf("Time zone: %ws\n", tzname);
					::WindowsDeleteString(hTimeZone);
				}
			}
		}
		spInst = nullptr;
	}

	::RoUninitialize();
}

HRESULT DisplayInterfaces(IInspectable* pInst) {
	HSTRING hName;
	if (SUCCEEDED(pInst->GetRuntimeClassName(&hName))) {
		ULONG32 len;
		auto name = ::WindowsGetStringRawBuffer(hName, &len);
		if (name)
			printf("Class name: %ws\n", name);
		::WindowsDeleteString(hName);
	}

	ULONG count;
	IID* iid;
	auto hr = pInst->GetIids(&count, &iid);
	if(FAILED(hr))
		return hr;

	WCHAR siid[64];
	WCHAR name[256];
	for(ULONG i = 0; i < count; i++) {
		if(SUCCEEDED(::StringFromGUID2(iid[i], siid, _countof(siid)))) {
			printf("I: %ws", siid);
			CRegKey key;
			if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, 
				(std::wstring(L"\\Interface\\") + siid).c_str(), KEY_QUERY_VALUE)) {
				ULONG chars = _countof(name);
				if(ERROR_SUCCESS == key.QueryStringValue(L"", name, &chars)) {
					printf(" %ws", name);
				}
			}
			printf("\n");
		}
	}
	::CoTaskMemFree(iid);
	return S_OK;
}
