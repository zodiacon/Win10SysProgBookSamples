// ProcessEnvironmentVariablesModificated.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>

WCHAR text[8192] = { '\0' };  // C6262

int main(int argc, char* argv[])
{
	PWSTR env = ::GetEnvironmentStrings();
	auto p = env;
	if (p) {  

		while (*p) {   // C28183
			auto equals = wcschr(p, L'=');

			if (equals != p && equals > 0) {  // C6387
				wcsncat_s(text, p, equals - p);
				wcscat_s(text, L": ");
				wcscat_s(text, equals + 1);   // C6387
				wcscat_s(text, L"\n");
			}
			p += wcslen(p) + 1;   // C6387
		}

		::FreeEnvironmentStrings(env);   // C28183
	}

	printf("%ls", text);

	return 0;
}