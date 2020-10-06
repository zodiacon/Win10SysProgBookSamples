#include <stdio.h>
#include <atlcomcli.h>
#include "JobNotifications.h"

const char* JobStateToString(BG_JOB_STATE state) {
    switch (state) {
        case BG_JOB_STATE_CONNECTING: return "Connecting";
        case BG_JOB_STATE_QUEUED: return "Queued";
        case BG_JOB_STATE_CANCELLED: return "Canceled";
        case BG_JOB_STATE_ERROR: return "Error";
        case BG_JOB_STATE_TRANSFERRED: return "Transferred";
        case BG_JOB_STATE_TRANSFERRING: return "Transferring";
        case BG_JOB_STATE_TRANSIENT_ERROR: return "Transient Error";
        case BG_JOB_STATE_ACKNOWLEDGED: return "Acknowledged";
        case BG_JOB_STATE_SUSPENDED: return "Suspended";
    }

    return "Unknown";
}

JobNotifications::JobNotifications(HANDLE hEvent) : _hEvent(hEvent) {
}

HRESULT __stdcall JobNotifications::QueryInterface(REFIID riid, void** ppvObject) {
    if (ppvObject == nullptr)
        return E_POINTER;

    if (riid == __uuidof(IUnknown) || riid == __uuidof(IBackgroundCopyCallback)) {
        // interface supported
        AddRef();
        *ppvObject = static_cast<IBackgroundCopyCallback*>(this);
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG __stdcall JobNotifications::AddRef(void) {
    return ++_refCount;
}

ULONG __stdcall JobNotifications::Release(void) {
    ULONG count = --_refCount;
    if (count == 0)
        delete this;

    return count;
}

HRESULT __stdcall JobNotifications::JobTransferred(IBackgroundCopyJob* pJob) {
    PWSTR name;
    pJob->GetDisplayName(&name);
    printf("Job %ws completed successfully!\n", name);
    ::CoTaskMemFree(name);
    pJob->Complete();
    pJob->SetNotifyInterface(nullptr);
    if (_hEvent)
        ::SetEvent(_hEvent);

    return S_OK;
}

HRESULT __stdcall JobNotifications::JobError(IBackgroundCopyJob* pJob, IBackgroundCopyError* pError) {
    PWSTR name, error, filename = nullptr;
    pJob->GetDisplayName(&name);
    pError->GetErrorDescription(LANGIDFROMLCID(::GetThreadLocale()), &error);
    CComPtr<IBackgroundCopyFile> spFile;
    pError->GetFile(&spFile);
    if (spFile)
        spFile->GetRemoteName(&filename);

    printf("Job %ws failed: %ws (file: %ws)\n", name, error, filename ? filename : L"(unknown)");
    ::CoTaskMemFree(name);
    ::CoTaskMemFree(error);
    if (filename)
        ::CoTaskMemFree(filename);

    pJob->SetNotifyInterface(nullptr);
    if (_hEvent)
        ::SetEvent(_hEvent);
    return S_OK;
}

HRESULT __stdcall JobNotifications::JobModification(IBackgroundCopyJob* pJob, DWORD dwReserved) {
    BG_JOB_STATE state;
    pJob->GetState(&state);
    PWSTR name;
    pJob->GetDisplayName(&name);
    printf("Job %ws changed to state: %s (%u)\n", name, JobStateToString(state), state);
    ::CoTaskMemFree(name);
    if (state == BG_JOB_STATE_CANCELLED) {
        pJob->SetNotifyInterface(nullptr);
        if (_hEvent)
            ::SetEvent(_hEvent);
    }
    return S_OK;
}
