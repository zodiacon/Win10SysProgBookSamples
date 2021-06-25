#include "pch.h"

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Networking::Connectivity;
using namespace std::literals::chrono_literals;
using namespace std::chrono;

IAsyncAction GetData() {
	auto now = clock::now();
	NetworkUsageStates states{ TriStates::DoNotCare, TriStates::DoNotCare };

	for (auto profile : NetworkInformation::GetConnectionProfiles()) {
		printf("Name: %ws\n", profile.ProfileName().c_str());
		printf("Before co_wait TID: %u\n", ::GetCurrentThreadId());
		auto usages = co_await profile.GetNetworkUsageAsync(now - 48h, now,
			DataUsageGranularity::PerDay, states);
		printf("After co_wait TID: %u\n", ::GetCurrentThreadId());
		for (auto usage : usages) {
			auto seconds = static_cast<unsigned>(duration_cast<std::chrono::seconds>(
				usage.ConnectionDuration()).count());
			if (seconds) {
				printf("Bytes Received = %llu\n", usage.BytesReceived());
				printf("Bytes Sent = %llu\n", usage.BytesSent());
				printf("Connection Duration = %u seconds\n\n", seconds);
			}
		}
	}
}

int main() {
	init_apartment();

	GetData().get();

	return 0;
}
