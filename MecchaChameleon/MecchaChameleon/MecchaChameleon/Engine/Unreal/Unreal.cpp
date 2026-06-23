#include "Unreal.hpp"
#include <cmath>

bool Unreal::WorldToScreen(const FMinimalViewInfo& viewInfo, FVector worldLocation, FVector2D& screenPos) {
    double radPitch = (viewInfo.Rotation.Pitch * 3.14159265358979 / 180.0);
    double radYaw = (viewInfo.Rotation.Yaw * 3.14159265358979 / 180.0);
    double radRoll = (viewInfo.Rotation.Roll * 3.14159265358979 / 180.0);

    double SP = sin(radPitch); double CP = cos(radPitch);
    double SY = sin(radYaw);   double CY = cos(radYaw);
    double SR = sin(radRoll);  double CR = cos(radRoll);

    FVector vAxisX = { CP * CY, CP * SY, SP };
    FVector vAxisY = { SR * SP * CY - CR * SY, SR * SP * SY + CR * CY, -SR * CP };
    FVector vAxisZ = { -(CR * SP * CY + SR * SY), CY * SR - CR * SP * SY, CR * CP };

    FVector vDelta = worldLocation - viewInfo.Location;

    double vTransformedX = vDelta.Dot(vAxisY);
    double vTransformedY = vDelta.Dot(vAxisZ);
    double vTransformedZ = vDelta.Dot(vAxisX);

    if (vTransformedZ < 0.1) return false;

    float ScreenWidth = (float)GetSystemMetrics(SM_CXSCREEN);
    float ScreenHeight = (float)GetSystemMetrics(SM_CYSCREEN);

    float tanFov = (float)(tan(viewInfo.FOV * 3.1415926535 / 360.0)) * 1.3f;

    float focalLength = (ScreenWidth / 2.0f) / tanFov;

    screenPos.x = (ScreenWidth / 2.0f) + (float)vTransformedX * focalLength / (float)vTransformedZ;
    screenPos.y = (ScreenHeight / 2.0f) - (float)vTransformedY * focalLength / (float)vTransformedZ;

    return true;
}