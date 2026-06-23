#ifndef ENGINE_TYPES_HPP
#define ENGINE_TYPES_HPP

#include <cstdint>

struct FVector {
    double x, y, z;

    FVector operator+(const FVector& v) const {
        return FVector{ x + v.x, y + v.y, z + v.z };
    }

    FVector operator-(const FVector& v) const {
        return FVector{ x - v.x, y - v.y, z - v.z };
    }

    FVector operator*(const FVector& v) const {
        return FVector{ x * v.x, y * v.y, z * v.z };
    }

    FVector operator/(const FVector& v) const {
        return FVector{ x / v.x, y / v.y, z / v.z };
    }

    double Dot(const FVector& v) const {
        return (x * v.x) + (y * v.y) + (z * v.z);
    }
};

struct FName {
    int32_t comparisonIndex;
    int32_t number;
};

struct TArray {
    uintptr_t data;
    int32_t count;
    int32_t max;
};

#endif // ENGINE_TYPES_HPP