// SafeStringFuntionsModificated.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

void wmain(int argc, const wchar_t* argv[])
{
    WCHAR buffer[32] = { '\0' };
    wcscpy_s(buffer, argv[1]);

    WCHAR* buffer2 = (WCHAR*)calloc(32, sizeof(WCHAR));

    if (buffer2) {
        wcscpy_s(buffer2, 32, argv[1]);
        free(buffer2);
    }
}