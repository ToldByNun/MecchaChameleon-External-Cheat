#ifndef HELPERS_HPP
#define HELPERS_HPP

#include "Memory/Memory.hpp"
#include "offsets.hpp"
#include <string>

class Helpers {
public:
    std::string resolveName(Memory& memory, uint32_t index) {
        uintptr_t namePool = Offsets::GNames;

        if (!namePool)
            return "None";

        uint32_t blockIndex = index >> 16;
        uint32_t offset = (index & 0xFFFF) << 1;

        uintptr_t blockPtr = memory.readMemory<uintptr_t>(
            namePool + 0x10 + (blockIndex * sizeof(uintptr_t))
        );

        if (!blockPtr)
            return "None";

        uintptr_t entryAddr = blockPtr + offset;

        uint16_t header = memory.readMemory<uint16_t>(entryAddr);

        bool isWide = header & 1;
        uint16_t length = header >> 6;

        if (length == 0 || length > 1024)
            return "None";

        if (isWide) {
            wchar_t buffer[1024] = {};

            memory.readRawMemory(
                entryAddr + sizeof(uint16_t),
                buffer,
                length * sizeof(wchar_t)
            );

            std::wstring ws(buffer, length);

            return std::string(ws.begin(), ws.end());
        }

        char buffer[1024] = {};

        memory.readRawMemory(
            entryAddr + sizeof(uint16_t),
            buffer,
            length
        );

        return std::string(buffer, length);
    }
};

#endif // HELPERS_HPP
