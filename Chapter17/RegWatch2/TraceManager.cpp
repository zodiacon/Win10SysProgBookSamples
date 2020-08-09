#include <Windows.h>
#include "TraceManager.h"

TraceManager::~TraceManager() {
	Stop();
}

bool TraceManager::Start(std::function<void(PEVENT_RECORD)> cb) {
	if (_handle || _hTrace)
		return true;

	_callback = cb;

	auto size = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(KERNEL_LOGGER_NAME);
	_propertiesBuffer = std::make_unique<BYTE[]>(size);
	::memset(_propertiesBuffer.get(), 0, size);

	_properties = reinterpret_cast<EVENT_TRACE_PROPERTIES*>(_propertiesBuffer.get());
	_properties->EnableFlags = EVENT_TRACE_FLAG_REGISTRY;
	_properties->Wnode.BufferSize = (ULONG)size;
	_properties->Wnode.Guid = SystemTraceControlGuid;
	_properties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
	_properties->Wnode.ClientContext = 1;
	_properties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
	_properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

	auto error = ::StartTrace(&_handle, KERNEL_LOGGER_NAME, _properties);
	if (error != ERROR_SUCCESS && error != ERROR_ALREADY_EXISTS)
		return false;

	WCHAR loggerName[] = KERNEL_LOGGER_NAME;
	_traceLog.Context = this;
	_traceLog.LoggerName = loggerName;
	_traceLog.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_REAL_TIME;
	_traceLog.EventRecordCallback = [](PEVENT_RECORD record) {
		((TraceManager*)record->UserContext)->OnEventRecord(record);
	};
	_hTrace = ::OpenTrace(&_traceLog);
	if (!_hTrace)
		return false;

	// create a dedicated thread to process the trace
	_hProcessThread.reset(::CreateThread(nullptr, 0, [](auto param) {
		return ((TraceManager*)param)->Run();
		}, this, 0, nullptr));

	return true;
}

bool TraceManager::Stop() {
	if (_hTrace) {
		::CloseTrace(_hTrace);
		_hTrace = 0;
	}
	if (_handle) {
		::StopTrace(_handle, KERNEL_LOGGER_NAME, _properties);
		_handle = 0;
	}
	return true;
}

void TraceManager::OnEventRecord(PEVENT_RECORD rec) {
	if (_callback)
		_callback(rec);
}

DWORD TraceManager::Run() {
	auto error = ::ProcessTrace(&_hTrace, 1, nullptr, nullptr);
	return error;
}
