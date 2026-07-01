#include "Aimbot.hpp"

#include <cmath>
#include <cfloat>
#include <Windows.h>

namespace {
	unsigned int getActiveHitboxMask() {
		const unsigned int mask = globals.settings.aimbot.hitboxMask;
		return mask != 0 ? mask : static_cast<unsigned int>(HitboxHead);
	}
}

void Aimbot::onAimbot(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo) {
	if (!globals.settings.aimbot.enabled) return;

	if (!(GetAsyncKeyState(globals.settings.aimbot.keybind) & 0x8000)) {
		this->lockedPawn = 0;
		return;
	}

	FVector2D bestAimPosition2D{};
	if (!this->getClosestTargetToCursor(actors, viewInfo, bestAimPosition2D))
		return;

	FVector2D step2D = this->applyAimSmoothing(bestAimPosition2D);
	if (step2D.x == 0 && step2D.y == 0) return;

	INPUT input = {};
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;
	input.mi.dx = static_cast<LONG>(step2D.x);
	input.mi.dy = static_cast<LONG>(step2D.y);
	SendInput(1, &input, sizeof(INPUT));
}

bool Aimbot::getBoneScreenPos(
	const TrackedActor& actor,
	int boneIndex,
	const FMinimalViewInfo& viewInfo,
	FVector2D& outScreenPos
) {
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

	if (boneIndex >= 0
		&& boneIndex < static_cast<int>(actor.boneList.size())
		&& actor.boneList.size() >= kSkeletonBoneCount) {
		const FVectorD& bone = actor.boneList[boneIndex];
		return this->unreal.WorldToScreen(
			viewInfo,
			FVector{ bone.x, bone.y, bone.z },
			outScreenPos,
			displaySize.x,
			displaySize.y
		);
	}

	FVector fallbackWorld = actor.location + FVector(0, 0, actor.playerSize);
	return this->unreal.WorldToScreen(
		viewInfo,
		fallbackWorld,
		outScreenPos,
		displaySize.x,
		displaySize.y
	);
}

bool Aimbot::getActorAimScreenPos(
	const TrackedActor& actor,
	const FMinimalViewInfo& viewInfo,
	FVector2D& outAimPosition2D
) {
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	const ImVec2 screenCenter(displaySize.x / 2.0f, displaySize.y / 2.0f);
	const unsigned int hitboxMask = getActiveHitboxMask();

	float closestDistance = FLT_MAX;
	bool found = false;

	for (HitboxFlags flag : kAllHitboxFlags) {
		if ((hitboxMask & flag) == 0)
			continue;

		FVector2D boneScreenPos{};
		if (!this->getBoneScreenPos(actor, HitboxFlagToBone(flag), viewInfo, boneScreenPos))
			continue;

		const float distance = std::hypot(
			boneScreenPos.x - screenCenter.x,
			boneScreenPos.y - screenCenter.y
		);
		if (distance >= closestDistance)
			continue;

		closestDistance = distance;
		outAimPosition2D = boneScreenPos;
		found = true;
	}

	return found;
}

bool Aimbot::isInFoV(const TrackedActor& actor, const FMinimalViewInfo& viewInfo) {
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	const ImVec2 screenCenter(displaySize.x / 2.0f, displaySize.y / 2.0f);
	FVector2D screenPos;

	if (!this->getActorAimScreenPos(actor, viewInfo, screenPos))
		return false;

	if (!globals.settings.aimbot.fovLimit) return true;

	const float distance = std::hypot(
		screenPos.x - screenCenter.x,
		screenPos.y - screenCenter.y
	);
	return distance <= globals.settings.aimbot.fov;
}

bool Aimbot::getClosestTargetToCursor(
	const std::vector<TrackedActor>& actors,
	const FMinimalViewInfo& viewInfo,
	FVector2D& outAimPosition2D
) {
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	const ImVec2 screenCenter(displaySize.x / 2.0f, displaySize.y / 2.0f);

	if (this->lockedPawn != 0) {
		for (const TrackedActor& actor : actors) {
			if (actor.pawn != this->lockedPawn) continue;

			if (actor.isLocalPlayer || actor.sameTeam || !this->isInFoV(actor, viewInfo)) {
				this->lockedPawn = 0;
				break;
			}

			if (this->getActorAimScreenPos(actor, viewInfo, outAimPosition2D))
				return true;

			this->lockedPawn = 0;
			break;
		}
	}

	float closestDistance = FLT_MAX;
	uintptr_t bestPawn = 0;
	bool foundTarget = false;

	for (const TrackedActor& actor : actors) {
		if (actor.isLocalPlayer || actor.sameTeam || !this->isInFoV(actor, viewInfo)) continue;

		FVector2D currentEnemyScreenPos{};
		if (!this->getActorAimScreenPos(actor, viewInfo, currentEnemyScreenPos)) continue;

		const float distance = std::hypot(
			currentEnemyScreenPos.x - screenCenter.x,
			currentEnemyScreenPos.y - screenCenter.y
		);
		if (distance > closestDistance) continue;

		closestDistance = distance;
		bestPawn = actor.pawn;
		outAimPosition2D = currentEnemyScreenPos;
		foundTarget = true;
	}

	this->lockedPawn = bestPawn;
	return foundTarget;
}

FVector2D Aimbot::applyAimSmoothing(const FVector2D& aimPosition2D) {
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	const FVector2D screenCenter = {
		displaySize.x / 2.0f,
		displaySize.y / 2.0f
	};

	FVector2D delta = {
		aimPosition2D.x - screenCenter.x,
		aimPosition2D.y - screenCenter.y
	};

	if (std::hypot(delta.x, delta.y) <= 1.f)
		return { 0, 0 };

	const float smooth = globals.settings.aimbot.smoothing ? globals.settings.aimbot.smooth : 1.f;
	return {
		delta.x / smooth,
		delta.y / smooth
	};
}
