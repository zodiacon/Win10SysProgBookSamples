#pragma once

#include <unordered_set>

class ProcessManager {
public:
	ProcessManager();

	bool Refresh();
	const std::vector<DWORD>& GetNewProcesses();

	std::vector<std::wstring> EnumModules(DWORD pid) const;

private:
	std::unordered_set<DWORD> _pids;
	std::vector<DWORD> _newPids;
};

