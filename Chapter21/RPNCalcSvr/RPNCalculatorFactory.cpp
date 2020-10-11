#include "pch.h"
#include "RPNCalculatorFactory.h"
#include "RPNCalculator.h"

HRESULT __stdcall RPNCalculatorFactory::QueryInterface(REFIID riid, void** ppvObject) {
    if (ppvObject == nullptr)
        return E_POINTER;

    if (riid == __uuidof(IUnknown) || riid == __uuidof(IClassFactory)) {
        *ppvObject = static_cast<IClassFactory*>(this);
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG __stdcall RPNCalculatorFactory::AddRef(void) {
    return 2;
}

ULONG __stdcall RPNCalculatorFactory::Release(void) {
    return 1;
}

HRESULT __stdcall RPNCalculatorFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject) {
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    auto calc = new RPNCalculator;
    auto hr = calc->QueryInterface(riid, ppvObject);
    calc->Release();

    return hr;
}

HRESULT __stdcall RPNCalculatorFactory::LockServer(BOOL fLock) {
    return E_NOTIMPL;
}
