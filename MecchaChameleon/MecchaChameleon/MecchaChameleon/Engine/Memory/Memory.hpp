#ifndef MEMORY_HPP
#define MEMORY_HPP
#include <Windows.h>
#include <cstdint>
#include <vector>#include <TlHelp32.h>
#include <iostream>
#include <cctype>
#include <cstring>
#include <optional>
#include <algorithm>
class Memory {
public:
	HANDLE processHandle;
	HWND windowHandle;

	uintptr_t baseAddress = 0;
	uintptr_t moduleSize = 0;
	uintptr_t gNames = 0;
	uintptr_t gWorld = 0;
	uint32_t processId = 0;
public:
	uint32_t getProcessIdByName(const char* processName);
	HWND findMainWindow();
	uintptr_t getModuleBaseAddress(const char* moduleName);
	bool attachToProcess(const char* processName);
	bool detachFromProcess();
	template <typename T>
	T readMemory(uintptr_t address) {
		T buffer{};
		ReadProcessMemory(processHandle, (LPCVOID)address, &buffer, sizeof(T), nullptr);
		return buffer;
	}
	bool readRawMemory(uintptr_t address, void* buffer, size_t size) {
		return ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), buffer, size, nullptr);
	}

	uintptr_t findAob(const char* pattern, uintptr_t start, size_t size) {
		const auto bytes = parsePattern(pattern);
		if (bytes.empty() || size < bytes.size())
			return 0;

		const size_t patternSize = bytes.size();
		const size_t chunkSize = 0x100000;
		std::vector<uint8_t> buffer(chunkSize + patternSize);

		for (size_t offset = 0; offset < size; ) {
			const size_t toRead = (std::min)(chunkSize + patternSize - 1, size - offset);
			if (!readRawMemory(start + offset, buffer.data(), toRead))
				return 0;

			const size_t scanLimit = toRead >= patternSize ? toRead - patternSize + 1 : 0;
			for (size_t i = 0; i < scanLimit; ++i) {
				bool match = true;

				for (size_t j = 0; j < patternSize; ++j) {
					if (bytes[j].has_value() && buffer[i + j] != bytes[j].value()) {
						match = false;
						break;
					}
				}

				if (match)
					return start + offset + i;
			}

			if (toRead <= patternSize)
				break;

			offset += toRead - (patternSize - 1);
		}

		return 0;
	}

	uintptr_t resolveAob(const char* pattern, uintptr_t instructionOffset, int relOffset = 3, int instructionSize = 7) {
		const uintptr_t match = findAob(pattern, baseAddress, moduleSize);
		if (!match)
			return 0;

		const uintptr_t instruction = match + instructionOffset;
		const int32_t relative = readMemory<int32_t>(instruction + relOffset);
		return instruction + instructionSize + relative;
	}

public:
	bool parsePatternByte(const char*& cursor, std::optional<uint8_t>& out) {
		while (*cursor == ' ')
			++cursor;

		if (*cursor == '\0')
			return false;

		if (*cursor == '?') {
			++cursor;
			if (*cursor == '?')
				++cursor;
			out = std::nullopt;
			return true;
		}

		char byteStr[3] = { cursor[0], cursor[1], '\0' };
		if (!std::isxdigit(static_cast<unsigned char>(byteStr[0])) ||
			!std::isxdigit(static_cast<unsigned char>(byteStr[1])))
			return false;

		out = static_cast<uint8_t>(strtoul(byteStr, nullptr, 16));
		cursor += 2;
		return true;
	}

	std::vector<std::optional<uint8_t>> parsePattern(const char* pattern) {
		std::vector<std::optional<uint8_t>> bytes;

		while (*pattern) {
			std::optional<uint8_t> byte;
			if (!parsePatternByte(pattern, byte))
				break;
			bytes.push_back(byte);
		}

		return bytes;
	}

};
#endif // MEMORY_HPP