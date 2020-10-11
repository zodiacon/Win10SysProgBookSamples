#include "pch.h"
#include "RPNCalculator.h"

HRESULT __stdcall RPNCalculator::Push(double value) {
    // limit the stack size
    if (_data.size() > 256)
        return E_UNEXPECTED;

    _data.push(value);
    return S_OK;
}

HRESULT __stdcall RPNCalculator::Pop(double* value) {
    if (_data.empty())
        return E_UNEXPECTED;

    *value = _data.top();
    _data.pop();

    return S_OK;
}

HRESULT __stdcall RPNCalculator::Add() {
    double x, y;
    auto hr = Pop(&x);
    if (FAILED(hr))
        return hr;

    hr = Pop(&y);
    if (FAILED(hr))
        return hr;

    Push(x + y);
    return S_OK;
}

HRESULT __stdcall RPNCalculator::Subtract() {
    double x, y;
    auto hr = Pop(&x);
    if (FAILED(hr))
        return hr;

    hr = Pop(&y);
    if (FAILED(hr))
        return hr;

    Push(y - x);
    return S_OK;
}

HRESULT __stdcall RPNCalculator::QueryInterface(REFIID riid, void** ppvObject) {
    if (ppvObject == nullptr)
        return E_POINTER;

    if (riid == __uuidof(IUnknown) || riid == __uuidof(IRPNCalculator)) {
        AddRef();
        *ppvObject = static_cast<IRPNCalculator*>(this);
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG __stdcall RPNCalculator::AddRef(void) {
    return ++_refCount;
}

ULONG __stdcall RPNCalculator::Release(void) {
    auto count = --_refCount;
    if (count == 0)
        delete this;
    return count;
}
