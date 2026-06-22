#ifndef ENGINE_TYPES_HPP
#define ENGINE_TYPES_HPP

#include <cstdint>

struct FName {
    int32_t comparisonIndex;
    int32_t number;
};

struct TArray {
    uintptr_t data;
    int32_t count;
    int32_t max;
};

#endif ENGINE_TYPES_HPP