#pragma once

#include <vector>
#include <unordered_map>
#include <memory>

struct ProcessKey {
	int64_t CreateTime;
	uint32_t Id;

	ProcessKey(int64_t create, uint32_t id) : CreateTime(create), Id(id) {}

	bool operator==(const ProcessKey& other) const {
		return CreateTime == other.CreateTime && Id == other.Id;
	}
};

template<>
struct std::hash<ProcessKey> {
	size_t operator()(const ProcessKey& key) const {
		return key.CreateTime ^ key.Id;
	}
};

struct ProcessInfo : ProcessKey {
	ProcessInfo(int64_t create, uint32_t id) : ProcessKey(create, id) {}

	CString Name;
	CString UserName;
	uint32_t SessionId;
	uint32_t ThreadCount;
	uint32_t HandleCount;
	int64_t KernelTime, UserTime;
	size_t WorkingSet;
};

class ProcessManager {
public:
	void Refresh();

	const std::vector<std::shared_ptr<ProcessInfo>>& GetProcesses() const;

	static CString GetUserNameFromPid(uint32_t pid);
	static CString GetUserNameFromSid(PSID sid);

private:
	std::vector<std::shared_ptr<ProcessInfo>> _processes;
	std::unordered_map<ProcessKey, std::shared_ptr<ProcessInfo>> _processesByKey;
};

