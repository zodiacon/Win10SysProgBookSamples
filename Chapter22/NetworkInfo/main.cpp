#include "pch.h"

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Networking::Connectivity;

int main() {
	init_apartment();

	for (auto profile : NetworkInformation::GetConnectionProfiles()) {
		printf("Name: %ws\n", profile.ProfileName().c_str());
	}

	return 0;
}
