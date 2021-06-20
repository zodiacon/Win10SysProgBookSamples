#include "pch.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Management::Deployment;

int main() {
	init_apartment();

	PackageManager pm;
	auto packages = pm.FindPackagesForUser(L"");
	int count = 0;
	for (auto package : packages) {
		if (package.IsFramework() || package.IsResourcePackage() || package.IsBundle())
			continue;

		count++;
		printf("%-40ws (%ws) %ws\n", 
			package.DisplayName().c_str(), 
			package.Id().FullName().c_str(), 
			package.Description().c_str());
	}
	printf("%d packages found.\n", count);

	return 0;
}
