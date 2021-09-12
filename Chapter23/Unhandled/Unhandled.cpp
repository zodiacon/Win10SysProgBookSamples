// Unhandled.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>

int Sample(int x) {
	int y = 0;
	x++;
	printf("About to divide 10 by %d\n", x);
	y = 10 / x;
	printf("10 / %d = %d\n", x, y);
	return y;
}

LONG WINAPI UnhandeldException(PEXCEPTION_POINTERS ep) {
	::Beep(800, 500);
	printf("Unhandled exception: 0x%X\n", ep->ExceptionRecord->ExceptionCode);

	return EXCEPTION_CONTINUE_SEARCH;
}

LONG WINAPI MyVehHandler(PEXCEPTION_POINTERS ep) {
	::Beep(1200, 500);
	printf("MyVehHandler 0x%X\n", ep->ExceptionRecord->ExceptionCode);
	return EXCEPTION_CONTINUE_SEARCH;
}

LONG WINAPI MyVehContHandler(PEXCEPTION_POINTERS ep) {
	::Beep(1000, 500);
	printf("Continue handler, unhandled 0x%X\n", ep->ExceptionRecord->ExceptionCode);
	return EXCEPTION_CONTINUE_EXECUTION;
}

int main() {
	::SetUnhandledExceptionFilter(UnhandeldException);
	::AddVectoredExceptionHandler(0, MyVehHandler);
	::AddVectoredContinueHandler(1, MyVehContHandler);

	Sample(4);
	Sample(-1);

	return 0;
}

