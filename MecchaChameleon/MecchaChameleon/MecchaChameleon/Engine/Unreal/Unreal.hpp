#ifndef UNREAL_HPP
#define UNREAL_HPP

#include "../types.hpp"
#include <Windows.h>

class Unreal {
public:
	bool WorldToScreen(const FMinimalViewInfo& viewInfo, FVector worldLocation, FVector2D& screenPos);
};

#endif // UNREAL_HPP