// Calculator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <atlcomcli.h>
#include "..\RPNCalcSvr\RPNCalcInterfaces.h"

int main() {
	::CoInitialize(nullptr);

	{
		CComPtr<IRPNCalculator> spCalc;
		auto hr = spCalc.CoCreateInstance(__uuidof(RPNCalculator));
		if (SUCCEEDED(hr)) {
			spCalc->Push(10);
			spCalc->Push(6);
			spCalc->Push(12);
			spCalc->Add();
			double result;
			spCalc->Pop(&result);
			printf("Value popped: %lf\n", result);
			spCalc->Push(result);
			spCalc->Subtract();
			spCalc->Pop(&result);
			printf("Value popped: %lf\n", result);
		}
	}

	::CoUninitialize();

	return 0;
}

