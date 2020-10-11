#pragma once

struct RPNCalculatorFactory : IClassFactory {
	// methods from IUnknown
	virtual HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
	virtual ULONG __stdcall AddRef(void) override;
	virtual ULONG __stdcall Release(void) override;
	// methods from IClassFactory
	virtual HRESULT __stdcall CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject) override;
	virtual HRESULT __stdcall LockServer(BOOL fLock) override;
};

