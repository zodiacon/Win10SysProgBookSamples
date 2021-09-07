// SEHDemo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>

int Sample1(int x) {
	int y = 0;
	__try {
		x++;
		y = 10 / x;
		printf("10 / %d = %d\n", x, y);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		printf("Handling exception 0x%X\n", GetExceptionCode());
	}
	return y;
}

int Sample2(int x) {
	int y = 0;
	__try {
		x++;
		y = 10 / x;
		printf("10 / %d = %d\n", x, y);
	}
	__except (GetExceptionCode() == EXCEPTION_INT_DIVIDE_BY_ZERO 
		? EXCEPTION_EXECUTE_HANDLER
		: EXCEPTION_CONTINUE_SEARCH) {
		printf("Handling divide by zero exception only\n");
	}
	return y;
}

int CanHandle(DWORD code) {
	if (code == EXCEPTION_INT_DIVIDE_BY_ZERO || code == EXCEPTION_FLT_DIVIDE_BY_ZERO)
		return EXCEPTION_EXECUTE_HANDLER;

	return EXCEPTION_CONTINUE_SEARCH;
}

int Sample3(int x) {
	int y = 0;
	__try {
		x++;
		y = 10 / x;
		printf("10 / %d = %d\n", x, y);
	}
	__except (CanHandle(GetExceptionCode())) {
		printf("Handling divide by zero exception only\n");
	}
	return y;
}

long long cellx = 1024, celly = 1024;
int cellSize = 256;

int FixMemory2(void* address, PEXCEPTION_POINTERS ep) {
	printf("Exception accessing memory 0x%p. Operation: %s\n",
		(PVOID)ep->ExceptionRecord->ExceptionInformation[1],
		ep->ExceptionRecord->ExceptionInformation[0] ? "Write" : "Read");

	printf("Committing memory at 0x%p\n", address);
	::VirtualAlloc(address, cellSize, MEM_COMMIT, PAGE_READWRITE);
	return EXCEPTION_CONTINUE_EXECUTION;
}

void WriteString2(void* buffer, int x, int y, const char* text) {
	char* p = nullptr;
	__try {
		p = (char*)buffer + (x + y * cellx) * cellSize;
		strcpy_s(p, strlen(text) + 1, text);
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
		? FixMemory2(p, GetExceptionInformation())
		: EXCEPTION_CONTINUE_SEARCH) {
		// will never run
	}
}

int FixMemory(void* address) {
	printf("Committing memory at 0x%p\n", address);
	::VirtualAlloc(address, cellSize, MEM_COMMIT, PAGE_READWRITE);
	return EXCEPTION_CONTINUE_EXECUTION;
}

void WriteString(void* buffer, int x, int y, const char* text) {
	char* p = nullptr;
	__try {
		p = (char*)buffer + (x + y * cellx) * cellSize;
		strcpy_s(p, strlen(text) + 1, text);
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
		? FixMemory(p)
		: EXCEPTION_CONTINUE_SEARCH) {
		// will never run
	}
}

void PrintString(void* buffer, int x, int y) {
	__try {
		printf("%s\n", (const char*)buffer + (x + y * cellx) * cellSize);
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
		? EXCEPTION_EXECUTE_HANDLER
		: EXCEPTION_CONTINUE_SEARCH) {
		printf("<memory cannot be accessed>\n");
	}
}

void MiniExcel() {
	BYTE* buffer = (BYTE*)::VirtualAlloc(nullptr, cellx * celly * cellSize, MEM_RESERVE, PAGE_READWRITE);

	WriteString2(buffer, 10, 20, "Hello, world1");
	WriteString2(buffer, 11, 20, "Hello, world2");
	WriteString2(buffer, 223, 90, "Hello, world3");
	WriteString2(buffer, 5, 788, "Hello, world4");

	PrintString(buffer, 223, 90);
	PrintString(buffer, 222, 90);
	PrintString(buffer, 11, 20);
	PrintString(buffer, 11, 21);
}

int x;
volatile int y;

__declspec(noinline)
int FixException() {
	printf("Fixing...\n");
	y = 1;
	return EXCEPTION_CONTINUE_EXECUTION;
}

void DoWork() {
	__try {
		int z = x / y;
		printf("z = %d\n", z);
	}
	__except (GetExceptionCode() == EXCEPTION_INT_DIVIDE_BY_ZERO
		? FixException() : EXCEPTION_CONTINUE_SEARCH) {
	}
}


int main() {
	printf("Result: %d\n", Sample1(1));
	printf("Result: %d\n", Sample1(-6));
	printf("Result: %d\n", Sample1(-1));

	Sample2(-1);

	MiniExcel();

	return 0;
}
