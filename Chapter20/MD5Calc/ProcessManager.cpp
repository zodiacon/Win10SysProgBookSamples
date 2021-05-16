#include "pch.h"
#include "ProcessManager.h"

ProcessManager::ProcessManager() {
	_pids.reserve(500);
	_newPids.reserve(500);
	Refresh();
}

bool ProcessManager::Refresh() {
	auto old = _pids;

	_newPids.clear();

	wil::unique_handle hSnapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
	if(!hSnapshot)
		return false;

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);

	if(!::Process32First(hSnapshot.get(), &pe))
		return false;

	while(::Process32Next(hSnapshot.get(), &pe)) {
		auto pid = pe.th32ProcessID;
		if(old.find(pid) == old.end()) {
			_newPids.push_back(pid);
			_pids.insert(pid);
		}	
		else {
			old.erase(pid);
		}
	}

	for(auto pid : old)
		_pids.erase(pid);

	return true;
}

const std::vector<DWORD>& ProcessManager::GetNewProcesses() {
	return _newPids;
}

std::vector<std::wstring> ProcessManager::EnumModules(DWORD pid) const {
	std::vector<std::wstring> modules;

	wil::unique_handle hSnapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid));
	if(!hSnapshot)
		return modules;

	MODULEENTRY32 me;
	me.dwSize = sizeof(me);

	if(!::Module32First(hSnapshot.get(), &me))
		return modules;

	modules.reserve(50);
	do {
		modules.push_back(me.szExePath);
	} while(::Module32Next(hSnapshot.get(), &me));

	return modules;
}
