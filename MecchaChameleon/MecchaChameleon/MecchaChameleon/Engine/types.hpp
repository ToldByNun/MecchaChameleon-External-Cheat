#ifndef ENGINE_TYPES_HPP
#define ENGINE_TYPES_HPP

#include <cstdint>
#include "Memory/Memory.hpp"

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

struct TArray {
    uintptr_t data;
    int32_t count;
    int32_t max;
};

struct FName {
    int32_t comparisonIndex;
    int32_t number;
};

struct FString {
    wchar_t* data;
    int32_t count;
    int32_t max;
};

struct FRotator {
    double Pitch, Yaw, Roll;
};

struct FMinimalViewInfo {
    FVector Location;
    FRotator Rotation;
    float FOV;
    float DesiredFOV;
};

struct PVelocity {
    double x, y;
};

struct D3DMATRIX {
    union {
        struct {
            double _11, _12, _13, _14;
            double _21, _22, _23, _24;
            double _31, _32, _33, _34;
            double _41, _42, _43, _44;
        };
        double m[4][4];
    };
};

struct FVector2D {
    double x, y;

    FVector2D() : x(0), y(0) {}
    FVector2D(double _x, double _y) : x(_x), y(_y) {}

    bool isValid() {
        return x > 0 && y > 0;
    }
};

struct FVectorD
{
    double x, y, z;
};

struct FQuat { double x, y, z, w; };
struct FTransform {
    FQuat Rotation;
    FVector Translation;
    char pad[0x8];
    FVector Scale3D;
};

struct FTransformD
{
    FQuat rotation;
    FVectorD translation;
    FVectorD scale3d;
};

static FVectorD RotateVector(const FQuat& q, const FVectorD& v)
{
    FVectorD qv{ q.x, q.y, q.z };

    FVectorD uv{
        qv.y * v.z - qv.z * v.y,
        qv.z * v.x - qv.x * v.z,
        qv.x * v.y - qv.y * v.x
    };

    FVectorD uuv{
        qv.y * uv.z - qv.z * uv.y,
        qv.z * uv.x - qv.x * uv.z,
        qv.x * uv.y - qv.y * uv.x
    };

    uv.x *= 2.0 * q.w;
    uv.y *= 2.0 * q.w;
    uv.z *= 2.0 * q.w;

    uuv.x *= 2.0;
    uuv.y *= 2.0;
    uuv.z *= 2.0;

    return {
        v.x + uv.x + uuv.x,
        v.y + uv.y + uuv.y,
        v.z + uv.z + uuv.z
    };
}

inline constexpr int kSkeletonBoneCount = 28;
inline constexpr int kMaxTrackedPlayers = 128;

enum BoneParts {
	AMM,
	LOOT,
	SPINE1,
	SPINE2,
	SPINE3,
	NECK,
	HEAD,
	HEAD_END,
	SHOULDER_L,
	UPPER_ARM_L,
	LOWER_ARM_L,
	HAND_L,
	HAND_L_END,
	SHOULDER_R,
	UPPER_ARM_R,
	LOWER_ARM_R,
	HAND_R,
	HAND_R_END,
	HIP_L,
	UPPER_LEG_L,
	LOWER_LEG_L,
	FOOT_L,
	FOOT_L_END,
	HIP_R,
	UPPER_LEG_R,
	LOWER_LEG_R,
	FOOT_R,
	FOOT_R_END
};

// Bit order must match hitboxOptions[] in Menu.cpp.
enum HitboxFlags : unsigned int {
	HitboxHead      = 1u << 0,
	HitboxChest     = 1u << 1,
	HitboxStomach   = 1u << 2,
	HitboxHip       = 1u << 3,
	HitboxLeftArm   = 1u << 4,
	HitboxRightArm  = 1u << 5,
	HitboxLeftKnee  = 1u << 6,
	HitboxRightKnee = 1u << 7,
};

inline constexpr HitboxFlags kAllHitboxFlags[] = {
	HitboxHead,
	HitboxChest,
	HitboxStomach,
	HitboxHip,
	HitboxLeftArm,
	HitboxRightArm,
	HitboxLeftKnee,
	HitboxRightKnee,
};

inline constexpr int kHitboxFlagCount = sizeof(kAllHitboxFlags) / sizeof(kAllHitboxFlags[0]);

inline constexpr int HitboxFlagToBone(HitboxFlags flag) {
	switch (flag) {
	case HitboxHead:      return HEAD;
	case HitboxChest:     return SPINE3;
	case HitboxStomach:   return SPINE2;
	case HitboxHip:       return LOOT;
	case HitboxLeftArm:   return LOWER_ARM_L;
	case HitboxRightArm:  return LOWER_ARM_R;
	case HitboxLeftKnee:  return LOWER_LEG_L;
	case HitboxRightKnee: return LOWER_LEG_R;
	default:              return HEAD;
	}
}

#endif // ENGINE_TYPES_HPP