#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <Windows.h>
#include <vector>

class Memory {
public:

	HANDLE processHandle;
	HWND windowHandle;
	uintptr_t baseAddress;

public:

	uint32_t getProcessIdByName(const char* processName);

	uintptr_t getModuleBaseAddress(const char* moduleName);

	bool attachToProcess(const char* processName);
	bool detachFromProcess();

	template <typename T>
	T readMemory(uintptr_t address) {
		T buffer;
		ReadProcessMemory(processHandle, (LPCVOID)address, &buffer, sizeof(T), nullptr);
		return buffer;
	}

	bool readRawMemory(uintptr_t address, void* buffer, size_t size) {
		return ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), buffer, size, nullptr);
	}

};

#endif MEMORY_HPP