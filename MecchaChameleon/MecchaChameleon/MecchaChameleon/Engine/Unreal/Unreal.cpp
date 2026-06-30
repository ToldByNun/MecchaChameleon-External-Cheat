#include "Unreal.hpp"
#include <cmath>

bool Unreal::WorldToScreen(const FMinimalViewInfo& viewInfo, FVector worldLocation, FVector2D& screenPos, float screenWidth, float screenHeight) {
    if (screenWidth <= 0.f || screenHeight <= 0.f)
        return false;

    const double Pi = 3.14159265358979;
    const double RotToRad = Pi / 180.0;

    double radPitch = viewInfo.Rotation.Pitch * RotToRad;
    double radYaw = viewInfo.Rotation.Yaw * RotToRad;

    double radYawY = (viewInfo.Rotation.Yaw + 89.8) * RotToRad;

    double radPitchZ = (viewInfo.Rotation.Pitch + 89.8) * RotToRad;
    double radYawZ = viewInfo.Rotation.Yaw * RotToRad;

    FVector vAxisX = { cos(radYaw) * cos(radPitch), sin(radYaw) * cos(radPitch), sin(radPitch) };
    double xLen = sqrt(vAxisX.x * vAxisX.x + vAxisX.y * vAxisX.y + vAxisX.z * vAxisX.z);
    if (xLen != 0.0) { vAxisX.x /= xLen; vAxisX.y /= xLen; vAxisX.z /= xLen; }

    FVector vAxisY = { cos(radYawY), sin(radYawY), 0.0 };
    double yLen = sqrt(vAxisY.x * vAxisY.x + vAxisY.y * vAxisY.y);
    if (yLen != 0.0) { vAxisY.x /= yLen; vAxisY.y /= yLen; }
    vAxisY.z = 0.0;

    FVector vAxisZ = { cos(radYawZ) * cos(radPitchZ), sin(radYawZ) * cos(radPitchZ), sin(radPitchZ) };
    double zLen = sqrt(vAxisZ.x * vAxisZ.x + vAxisZ.y * vAxisZ.y + vAxisZ.z * vAxisZ.z);
    if (zLen != 0.0) { vAxisZ.x /= zLen; vAxisZ.y /= zLen; vAxisZ.z /= zLen; }

    FVector vDelta = worldLocation - viewInfo.Location;

    double vTransformedX = vDelta.Dot(vAxisY);
    double vTransformedY = vDelta.Dot(vAxisZ);
    double vTransformedZ = vDelta.Dot(vAxisX);

    if (vTransformedZ < 1.0) return false;

    float tanFov = (float)(tan(viewInfo.FOV * Pi / 360.0));
    float focalLength = (screenWidth / 2.0f) / tanFov;

    screenPos.x = (screenWidth / 2.0f) + (float)vTransformedX * focalLength / (float)vTransformedZ;
    screenPos.y = (screenHeight / 2.0f) - (float)vTransformedY * focalLength / (float)vTransformedZ;

    return true;
}