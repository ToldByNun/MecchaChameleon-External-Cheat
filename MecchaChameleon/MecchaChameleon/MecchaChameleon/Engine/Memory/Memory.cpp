#include "Memory.hpp"
#include <TlHelp32.h>
#include <iostream>
#include <cctype>
#include <cstring>
#include <optional>
#include <algorithm>

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

	this->processId = processId;

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
			moduleSize = moduleEntry.modBaseSize;
		}
		CloseHandle(snapshot);
	}

	if (baseAddress == 0) {
		std::cerr << "[!] BaseAddress could not be located." << std::endl;;
		return false;
	}

	windowHandle = findMainWindow();

	std::cout << "[+] Successfully attached to " << processName << "!" << std::endl;
	std::cout << "[+] PID: " << processId << " | Base: 0x" << std::hex << baseAddress << std::dec << std::endl;
	if (windowHandle)
		std::cout << "[+] Game window found." << std::endl;
	else
		std::cout << "[!] Game window not found — overlay may fail." << std::endl;

	return true;
}

HWND Memory::findMainWindow() {
	if (processId == 0)
		return nullptr;

	struct EnumData {
		DWORD pid;
		HWND hwnd;
		LONG bestArea;
	} data = { processId, nullptr, 0 };

	EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
		auto* d = reinterpret_cast<EnumData*>(lParam);

		DWORD windowPid = 0;
		GetWindowThreadProcessId(hwnd, &windowPid);
		if (windowPid != d->pid || !IsWindowVisible(hwnd))
			return TRUE;

		RECT rect{};
		GetWindowRect(hwnd, &rect);
		const LONG area = (rect.right - rect.left) * (rect.bottom - rect.top);
		if (area > d->bestArea) {
			d->bestArea = area;
			d->hwnd = hwnd;
		}
		return TRUE;
	}, reinterpret_cast<LPARAM>(&data));

	return data.hwnd;
}

bool Memory::detachFromProcess() {
	if (processHandle) {
		CloseHandle(processHandle);
		processHandle = nullptr;

		return true;
	}

	return false;
}