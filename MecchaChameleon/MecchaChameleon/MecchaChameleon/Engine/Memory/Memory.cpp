#include "Memory.hpp"
#include <TlHelp32.h>
#include <iostream>

uint32_t Memory::getProcessIdByName(const char* processName) {
	uint32_t processId = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (snapshot != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32 processEntry;
		processEntry.dwSize = sizeof(PROCESSENTRY32);

		if (Process32First(snapshot, &processEntry)) {
			do {
				if (strcmp(processEntry.szExeFile, processName) == 0) {
					processId = processEntry.th32ProcessID;
					break;
				}
			} while (Process32Next(snapshot, &processEntry));
		}
		CloseHandle(snapshot);
	}

	return processId;
}

bool Memory::attachToProcess(const char* processName) {
	uint32_t processId = getProcessIdByName(processName);
	if (processId == 0) {
		std::cerr << "[-] Process not found: " << processName << std::endl;
		return false;
	}

	processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	if (processHandle == nullptr) {
		std::cerr << "[-] Failed to open process: " << GetLastError() << std::endl;
		return false;
	}

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
	if (snapshot != INVALID_HANDLE_VALUE) {
		MODULEENTRY32 moduleEntry;
		moduleEntry.dwSize = sizeof(moduleEntry);
		if (Module32First(snapshot, &moduleEntry)) {
			baseAddress = reinterpret_cast<uintptr_t>(moduleEntry.modBaseAddr);
		}
		CloseHandle(snapshot);
	}

	if (baseAddress == 0) {
		std::cerr << "[!] BaseAddress could not be located." << std::endl;;
		return false;
	}

	std::cout << "[+] Successfully attached to " << processName << "!" << std::endl;
	std::cout << "[+] PID: " << processId << " | Base: 0x" << std::hex << baseAddress << std::dec << std::endl;

	return true;
}

bool Memory::detachFromProcess() {
	if (processHandle) {
		CloseHandle(processHandle);
		processHandle = nullptr;

		return true;
	}

	return false;
}