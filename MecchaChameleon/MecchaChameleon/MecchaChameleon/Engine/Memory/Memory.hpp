#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <string>
#include <vector>

struct PatternByte {
	uint8_t value;
	bool wildcard;
};

enum class RipMode {
	None,
	Lea,
	Mov
};

struct AobResult {
	uintptr_t match = 0;
	uintptr_t instruction = 0;
	uintptr_t target = 0;
	uintptr_t value = 0;
};

class Memory {
public:
	HANDLE processHandle = nullptr;
	HWND windowHandle = nullptr;

	uintptr_t baseAddress = 0;
	uintptr_t moduleSize = 0;
	uint32_t processId = 0;

	uint32_t getProcessIdByName(const char* processName);
	HWND findMainWindow();
	uintptr_t getModuleBaseAddress(const char* moduleName);

	bool attachToProcess(const char* processName);
	bool detachFromProcess();

	template <typename T>
	T readMemory(uintptr_t address) {
		T buffer{};
		ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), &buffer, sizeof(T), nullptr);
		return buffer;
	}

	bool readRawMemory(uintptr_t address, void* buffer, size_t size) {
		return ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), buffer, size, nullptr);
	}

	std::vector<PatternByte> parsePattern(const std::string& pattern);
	uintptr_t findAob(uintptr_t start, size_t size, const std::string& pattern);
	uintptr_t resolveRip(uintptr_t instruction, int relOffset = 0x3, int instructionSize = 0x7);
	AobResult resolveAob(const std::string& pattern, uintptr_t instructionOffset, RipMode mode, int relOffset = 0x3, int instructionSize = 0x7);
};

#endif // MEMORY_HPP
