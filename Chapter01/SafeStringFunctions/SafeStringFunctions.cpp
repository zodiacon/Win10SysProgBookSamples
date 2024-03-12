// SafeStringFunctions.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

void wmain(int argc, const wchar_t* argv[])
{
    // assume argc >= 2 for this demo

    WCHAR buffer[32];
    wcscpy_s(buffer, argv[1]);       // C++ version aware of static buffers

    WCHAR* buffer2 = (WCHAR*)malloc(32 * sizeof(WCHAR));
    // wcscpy_s(buffer, argv[1]);         // does not compile 
    wcscpy_s(buffer2, 32, argv[1]);          // size in characters (not bytes)

    free(buffer2);
}