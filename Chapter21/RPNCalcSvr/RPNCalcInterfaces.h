#pragma once

struct __declspec(uuid("7020B8D9-CEFE-46DC-89D7-F0261C3CDE66")) IRPNCalculator : IUnknown {
	virtual HRESULT __stdcall Push(double value) = 0;
	virtual HRESULT __stdcall Pop(double* value) = 0;
	virtual HRESULT __stdcall Add() = 0;
	virtual HRESULT __stdcall Subtract() = 0;
};

// {7020B8D9-CEFE-46DC-89D7-F0261C3CDE66}
DEFINE_GUID(IID_IRPNCalculator,
	0x7020b8d9, 0xcefe, 0x46dc, 0x89, 0xd7, 0xf0, 0x26, 0x1c, 0x3c, 0xde, 0x66);

// {FA523D4E-DB35-4D0B-BD0A-002281FE3F31}
DEFINE_GUID(CLSID_RPNCalculator,
	0xfa523d4e, 0xdb35, 0x4d0b, 0xbd, 0xa, 0x0, 0x22, 0x81, 0xfe, 0x3f, 0x31);

class __declspec(uuid("FA523D4E-DB35-4D0B-BD0A-002281FE3F31")) RPNCalculator;
