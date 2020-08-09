#pragma once

#define INITGUID

#include <functional>
#include <tdh.h>
#include <memory>
#include <wil\resource.h>

class TraceManager final {
public:
	~TraceManager();

	bool Start(std::function<void(PEVENT_RECORD)> callback);
	bool Stop();

private:
	void OnEventRecord(PEVENT_RECORD rec);
	DWORD Run();

private:
	TRACEHANDLE _handle{ 0 };
	TRACEHANDLE _hTrace{ 0 };
	EVENT_TRACE_PROPERTIES* _properties;
	std::unique_ptr<BYTE[]> _propertiesBuffer;
	EVENT_TRACE_LOGFILE _traceLog = { 0 };
	wil::unique_handle _hProcessThread;
	std::function<void(PEVENT_RECORD)> _callback;
};

