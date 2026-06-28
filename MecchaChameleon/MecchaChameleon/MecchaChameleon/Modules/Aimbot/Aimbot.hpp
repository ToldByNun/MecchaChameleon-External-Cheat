#ifndef AIMBOT_HPP
#define AIMBOT_HPP

#include "../../Manager/Classmanager/Classmanager.hpp"
#include "../../Manager/Globals/Globals.hpp"
#include "../../Engine/types.hpp"
#include "../../Engine/MecchaChameleon/MecchaChameleon.hpp"
#include "../../Engine/Unreal/Unreal.hpp"

class Aimbot : public IManagedClass {
public:
	Unreal unreal;

	void onAimbot(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo);

private:
	bool isInFoV(const TrackedActor& actor, const FMinimalViewInfo& viewInfo);
	bool isAimTarget(const TrackedActor& actor);
	bool getActorHeadScreenPos(const TrackedActor& actor, const FMinimalViewInfo& viewInfo, FVector2D& outHeadPosition2D);
	bool getClosestTargetToCursor(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo, FVector2D& outHeadPosition2D);

private:
	// extras
	uintptr_t lockedPawn = 0;
	FVector2D applyAimSmoothing(const FVector2D& headPosition2D);
};

#endif // AIMBOT_HPP
