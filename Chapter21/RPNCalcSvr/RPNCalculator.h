#pragma once

#include <stack>
#include "RPNCalcInterfaces.h"

class RPNCalculator : public IRPNCalculator {
public:
	// from IUnknown
	virtual HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
	virtual ULONG __stdcall AddRef(void) override;
	virtual ULONG __stdcall Release(void) override;
	// from IRPNCalculator
	virtual HRESULT __stdcall Push(double value) override;
	virtual HRESULT __stdcall Pop(double* value) override;
	virtual HRESULT __stdcall Add() override;
	virtual HRESULT __stdcall Subtract() override;

private:
	ULONG _refCount{ 1 };
	std::stack<double> _data;
};

