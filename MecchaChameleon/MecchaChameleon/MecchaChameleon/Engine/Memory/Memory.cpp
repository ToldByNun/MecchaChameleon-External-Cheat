#include "Memory.hpp"
#include <TlHelp32.h>
#include <iostream>
#include <cctype>
#include <cstring>
#include <optional>
#include <algorithm>
#include <sstream>

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

std::string Memory::readFString(uintptr_t address) {
	FString str = readMemory<FString>(address);

	if (!str.data || str.count <= 0 || str.count > 128 || str.max < str.count)
		return "";

	std::wstring wide;
	wide.resize(str.count);

	if (!ReadProcessMemory(
		processHandle,
		reinterpret_cast<LPCVOID>(str.data),
		wide.data(),
		str.count * sizeof(wchar_t),
		nullptr
	)) {
		return "";
	}

	if (!wide.empty() && wide.back() == L'\0')
		wide.pop_back();

	std::string out;
	out.reserve(wide.size());

	for (wchar_t c : wide) {
		if (c >= 0 && c <= 127)
			out.push_back(static_cast<char>(c));
		else
			out.push_back('?');
	}

	return out;
}

std::vector<PatternByte> Memory::parsePattern(const std::string& pattern) {
	std::vector<PatternByte> bytes;
	std::istringstream stream(pattern);
	std::string token;

	while (stream >> token) {
		if (token == "?" || token == "??") {
			bytes.push_back({ 0x00, true });
			continue;
		}

		bytes.push_back({
			static_cast<uint8_t>(std::stoul(token, nullptr, 16)),
			false
			});
	}

	return bytes;
}

uintptr_t Memory::findAob(uintptr_t start, size_t size, const std::string& pattern) {
	std::vector<PatternByte> pat = parsePattern(pattern);
	if (pat.empty() || size < pat.size()) return 0;

	std::vector<uint8_t> buffer(size);
	SIZE_T bytesRead = 0;

	if (!ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(start), buffer.data(), size, &bytesRead))
		return 0;

	for (size_t i = 0; i + pat.size() <= bytesRead; i++) {
		bool found = true;

		for (size_t j = 0; j < pat.size(); j++) {
			if (!pat[j].wildcard && buffer[i + j] != pat[j].value) {
				found = false;
				break;
			}
		}

		if (found)
			return start + i;
	}

	return 0;
}

uintptr_t Memory::resolveRip(uintptr_t instruction, int relOffset, int instructionSize) {
	int32_t rel = readMemory<int32_t>(instruction + relOffset);
	return instruction + instructionSize + rel;
}

AobResult Memory::resolveAob(
	const std::string& pattern,
	uintptr_t instructionOffset,
	RipMode mode,
	int relOffset,
	int instructionSize
) {
	AobResult result{};

	result.match = findAob(baseAddress, moduleSize, pattern);
	if (!result.match)
		return result;

	result.instruction = result.match + instructionOffset;
	result.target = resolveRip(result.instruction, relOffset, instructionSize);

	if (mode == RipMode::Mov)
		result.value = readMemory<uintptr_t>(result.target);
	else
		result.value = result.target;

	return result;
}