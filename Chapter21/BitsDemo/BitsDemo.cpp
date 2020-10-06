// BitsDemo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <Bits.h>
#include <atlcomcli.h>
#include "JobNotifications.h"

// only needed with using the full variables CLSID_BackgroundCopyManager and/or IID_IBackgroundCopyManager
//#pragma comment(lib, "bits")

int Error(HRESULT hr) {
	printf("COM error (hr=%08X)\n", hr);
	return 1;
}

HRESULT DoBITSWork0() {
	IBackgroundCopyManager* mgr;
	//HRESULT hr = ::CoCreateInstance(CLSID_BackgroundCopyManager, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&mgr));
	//HRESULT hr = ::CoCreateInstance(CLSID_BackgroundCopyManager, nullptr, CLSCTX_ALL, IID_IBackgroundCopyManager, reinterpret_cast<void**>(&mgr));
	HRESULT hr = ::CoCreateInstance(__uuidof(BackgroundCopyManager), nullptr, CLSCTX_ALL, __uuidof(IBackgroundCopyManager), reinterpret_cast<void**>(&mgr));

	if (FAILED(hr))
		return hr;

	GUID jobId;
	IBackgroundCopyJob* pJob;
	hr = mgr->CreateJob(L"My job 2", BG_JOB_TYPE_DOWNLOAD, &jobId, &pJob);
	if (FAILED(hr))
		return Error(hr);

	mgr->Release();

	hr = pJob->AddFile(L"https://www.fnordware.com/superpng/pnggrad16rgb.png", L"c:\\temp\\image.png");
	if (FAILED(hr))
		return Error(hr);

	hr = pJob->Resume();
	if (SUCCEEDED(hr)) {
		printf("Downloading... ");
		BG_JOB_STATE state;
		for (;;) {
			pJob->GetState(&state);
			if (state == BG_JOB_STATE_ERROR || state == BG_JOB_STATE_TRANSFERRED)
				break;
			printf(".");
			::Sleep(300);
		}
		if (state == BG_JOB_STATE_ERROR) {
			printf("\nError in transfer!\n");
		}
		else {
			pJob->Complete();
			printf("\nTransfer successful!\n");
		}
	}
	pJob->Release();

	return hr;
}

HRESULT DoBITSWork() {
	// assume CoInitialize has already been called for this thread

	CComPtr<IBackgroundCopyManager> spMgr;
	HRESULT hr = spMgr.CoCreateInstance(__uuidof(BackgroundCopyManager));
	if (FAILED(hr))
		return hr;

	CComPtr<IBackgroundCopyJob> spJob;
	GUID guid;
	hr = spMgr->CreateJob(L"My Job", BG_JOB_TYPE_DOWNLOAD, &guid, &spJob);
	if (FAILED(hr))
		return hr;

	hr = spJob->AddFile(L"https://www.fnordware.com/superpng/pnggrad16rgb.png", L"c:\\temp\\image.png");
	if (FAILED(hr))
		return hr;

	hr = spJob->Resume();
	if (SUCCEEDED(hr)) {
		printf("Downloading... ");
		BG_JOB_STATE state;
		for (;;) {
			spJob->GetState(&state);
			if (state == BG_JOB_STATE_ERROR || state == BG_JOB_STATE_TRANSFERRED)
				break;
			printf(".");
			::Sleep(300);
		}
		if (state == BG_JOB_STATE_ERROR) {
			printf("\nError in transfer!\n");
		}
		else {
			spJob->Complete();
			printf("\nTransfer successful!\n");
		}
	}
	return hr;
}

HRESULT DoBITSWork2() {
	// assume CoInitialize has already been called for this thread

	CComPtr<IBackgroundCopyManager> spMgr;
	HRESULT hr = spMgr.CoCreateInstance(__uuidof(BackgroundCopyManager));
	if (FAILED(hr))
		return hr;

	CComPtr<IBackgroundCopyJob> spJob;
	GUID guid;
	hr = spMgr->CreateJob(L"My Job", BG_JOB_TYPE_DOWNLOAD, &guid, &spJob);
	if (FAILED(hr))
		return hr;

	// leading space for SetNotifyCmdLine
	WCHAR localPath[] = L" c:\\temp\\image.png";

	CComPtr<IBackgroundCopyJob2> spJob2;
	hr = spJob->QueryInterface(__uuidof(IBackgroundCopyJob2), reinterpret_cast<void**>(&spJob2));
	if (spJob2) {	// checking HR is ok too
		hr = spJob2->SetNotifyCmdLine(L"c:\\windows\\system32\\mspaint.exe", localPath);
	}

	hr = spJob->AddFile(L"https://www.fnordware.com/superpng/pnggrad16rgb.png", localPath + 1);
	if (FAILED(hr))
		return hr;

	hr = spJob->Resume();
	if (SUCCEEDED(hr)) {
		printf("Downloading... ");
		BG_JOB_STATE state;
		for (;;) {
			spJob->GetState(&state);
			if (state == BG_JOB_STATE_ERROR || state == BG_JOB_STATE_TRANSFERRED)
				break;
			printf(".");
			::Sleep(300);
		}
		if (state == BG_JOB_STATE_ERROR) {
			printf("\nError in transfer!\n");
		}
		else {
			spJob->Complete();
			printf("\nTransfer successful!\n");
		}
	}
	return hr;
}

HRESULT DoBitsWorkWithNotify(PCWSTR jobName, PCWSTR remoteUrl, PCWSTR localFile, HANDLE hEvent = nullptr) {
	CComPtr<IBackgroundCopyManager> spMgr;
	HRESULT hr = spMgr.CoCreateInstance(__uuidof(BackgroundCopyManager));
	if (FAILED(hr))
		return hr;

	CComPtr<IBackgroundCopyJob> spJob;
	GUID guid;
	hr = spMgr->CreateJob(jobName, BG_JOB_TYPE_DOWNLOAD, &guid, &spJob);
	if (FAILED(hr))
		return hr;

	hr = spJob->AddFile(remoteUrl, localFile);
	if (FAILED(hr))
		return hr;

	spJob->SetNotifyFlags(BG_NOTIFY_JOB_TRANSFERRED | BG_NOTIFY_JOB_MODIFICATION 
		| BG_NOTIFY_FILE_RANGES_TRANSFERRED | BG_NOTIFY_JOB_ERROR);
	auto notify = new JobNotifications(hEvent);
	hr = spJob->SetNotifyInterface(notify);
	if (FAILED(hr))
		return hr;

	return spJob->Resume();
}


int main() {
	::CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	HANDLE hEvent[] = {
		::CreateEvent(nullptr, FALSE, FALSE, nullptr),
		::CreateEvent(nullptr, FALSE, FALSE, nullptr),
	};
	DoBitsWorkWithNotify(L"My first Job", L"http://speedtest.ftp.otenet.gr/files/test10Mb.db", L"c:\\temp\\test1.db", hEvent[0]);
	DoBitsWorkWithNotify(L"My second Job", L"http://speedtest.ftp.otenet.gr/files/test1Mb.db", L"c:\\temp\\test2.db", hEvent[1]);

	::WaitForMultipleObjects(_countof(hEvent), hEvent, TRUE, INFINITE);
	printf("All jobs completed!\n");

	::CloseHandle(hEvent[0]);
	::CloseHandle(hEvent[1]);

	::CoUninitialize();
	return 0;
}

