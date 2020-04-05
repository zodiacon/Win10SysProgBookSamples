// EnumDevices.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <SetupAPI.h>
#include <vector>
#include <string>
#define INITGUID
#include <Wiaintfc.h>
#include <Ntddvdeo.h>
#include <devpkey.h>
#include <Ntddkbd.h>

#pragma comment(lib, "setupapi")

struct DeviceInfo {
	std::wstring SymbolicLink;
	std::wstring FriendlyName;
};

std::vector<DeviceInfo> EnumDevices(const GUID& guid) {
	std::vector<DeviceInfo> devices;

	auto hInfoSet = ::SetupDiGetClassDevs(&guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
	if (hInfoSet == INVALID_HANDLE_VALUE)
		return devices;

	devices.reserve(4);

	SP_INTERFACE_DEVICE_DATA data = { sizeof(data) };
	SP_DEVINFO_DATA ddata = { sizeof(ddata) };
	BYTE buffer[1 << 12];
	for (DWORD i = 0; ; i++) {
		if (!::SetupDiEnumDeviceInterfaces(hInfoSet, nullptr, &guid, i, &data))
			break;
		auto details = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA*> (buffer);
		details->cbSize = sizeof(*details);
		if (::SetupDiGetDeviceInterfaceDetail(hInfoSet, &data, details, sizeof(buffer), nullptr, &ddata)) {
			DeviceInfo info;
			info.SymbolicLink = details->DevicePath;

			if(::SetupDiGetDeviceRegistryProperty(hInfoSet, &ddata, SPDRP_DEVICEDESC, nullptr, buffer, sizeof(buffer), nullptr))
				info.FriendlyName = (WCHAR*)buffer;

			devices.push_back(std::move(info));
		}
	}
	::SetupDiDestroyDeviceInfoList(hInfoSet);

	return devices;
}

void DisplayDevices(const std::vector<DeviceInfo>& devices, const char* name) {
	printf("%s\n%s\n", name, std::string(::strlen(name), '-').c_str());
	for (auto& di : devices) {
		printf("Symbolic link: %ws\n", di.SymbolicLink.c_str());
		printf("  Name: %ws\n", di.FriendlyName.c_str());
		auto hDevice = ::CreateFile(di.SymbolicLink.c_str(), GENERIC_READ, 
			FILE_SHARE_READ | FILE_SHARE_WRITE, 
			nullptr, OPEN_EXISTING, 0, nullptr);
		if (hDevice == INVALID_HANDLE_VALUE)
			printf("  Failed to open device (%d)\n", ::GetLastError());
		else {
			printf("  Device opened successfully!\n");
			::CloseHandle(hDevice);
		}
	}
	printf("\n");
}

int main() {
	auto devices = EnumDevices(GUID_DEVINTERFACE_IMAGE);
	DisplayDevices(devices, "Image");

	DisplayDevices(EnumDevices(GUID_DEVINTERFACE_MONITOR), "Monitor");
	DisplayDevices(EnumDevices(GUID_DEVINTERFACE_DISPLAY_ADAPTER), "Display Adapter");
	DisplayDevices(EnumDevices(GUID_DEVINTERFACE_DISK), "Disk");
	DisplayDevices(EnumDevices(GUID_DEVINTERFACE_KEYBOARD), "keyboard");

	return 0;
}
