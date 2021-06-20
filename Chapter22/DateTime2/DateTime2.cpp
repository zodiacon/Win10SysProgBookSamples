// DateTime2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Globalization;

int main() {
	winrt::init_apartment();

	{
		Calendar calendar;
		calendar.SetToNow();
		printf("The time is %02d:%02d:%02d\n", 
			calendar.Hour(), calendar.Minute(), calendar.Second());
		printf("Time zone: %ws\n", calendar.TimeZoneAsString().c_str());
	}
	winrt::uninit_apartment();
}
