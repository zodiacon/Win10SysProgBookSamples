// NetworkInfoRaw.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

using namespace ABI::Windows::Networking::Connectivity;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace Microsoft::WRL::Wrappers;	// for HStringReference

#pragma comment(lib, "runtimeobject.lib")

int main() {
	Initialize(RO_INIT_MULTITHREADED);	// from ABI::Windows::Foundation

	CComPtr<INetworkInformationStatics> spStatics;
	auto hr = RoGetActivationFactory(
		HStringReference(RuntimeClass_Windows_Networking_Connectivity_NetworkInformation).Get(),
		__uuidof(INetworkInformationStatics),
		reinterpret_cast<void**>(&spStatics));

	if (SUCCEEDED(hr)) {
		CComPtr<IVectorView<ConnectionProfile*>> spProfiles;
		spStatics->GetConnectionProfiles(&spProfiles);
		if (spProfiles) {
			unsigned count;
			spProfiles->get_Size(&count);
			for (unsigned i = 0; i < count; i++) {
				CComPtr<IConnectionProfile> spProfile;
				spProfiles->GetAt(i, &spProfile);
				HSTRING hName;
				spProfile->get_ProfileName(&hName);
				printf("Name: %ws\n", ::WindowsGetStringRawBuffer(hName, nullptr));
				::WindowsDeleteString(hName);
			}
		}
	}

	return 0;
}
