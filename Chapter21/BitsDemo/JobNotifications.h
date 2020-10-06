#pragma once

#include <Bits.h>

struct JobNotifications : IBackgroundCopyCallback {
	explicit JobNotifications(HANDLE hEvent = nullptr);

	virtual HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
	virtual ULONG __stdcall AddRef(void) override;
	virtual ULONG __stdcall Release(void) override;
	virtual HRESULT __stdcall JobTransferred(IBackgroundCopyJob* pJob) override;
	virtual HRESULT __stdcall JobError(IBackgroundCopyJob* pJob, IBackgroundCopyError* pError) override;
	virtual HRESULT __stdcall JobModification(IBackgroundCopyJob* pJob, DWORD dwReserved) override;

private:
	ULONG _refCount{ 0 };
	HANDLE _hEvent;
};

