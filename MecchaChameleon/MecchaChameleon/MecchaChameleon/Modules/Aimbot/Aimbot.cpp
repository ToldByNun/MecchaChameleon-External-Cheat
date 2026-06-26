#include "Aimbot.hpp"
#include <cmath>
#include <Windows.h>

void Aimbot::onAimbot(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo) {
	if (!globals.settings.aimbot.enabled) return;

	FVector2D bestActorHeadPosition2D = this->getClosestTargetToCursor(actors, viewInfo);
	FVector2D step2D = this->applyAimSmoothing(bestActorHeadPosition2D);

	if (step2D.x == 0 && step2D.y == 0) return;

	if (GetAsyncKeyState(globals.settings.aimbot.keybind)) {
		SetCursorPos(
			static_cast<int>(bestActorHeadPosition2D.x + step2D.x),
			static_cast<int>(bestActorHeadPosition2D.y + step2D.y)
		);
	}
}

bool Aimbot::isInFoV(const TrackedActor& actor, const FMinimalViewInfo& viewInfo) {
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	const ImVec2 screenCenter(displaySize.x / 2.0f, displaySize.y / 2.0f);
	FVector2D screenPos;

	bool bIsEnemyOnScreen = this->unreal.WorldToScreen(
		viewInfo,
		actor.location,
		screenPos,
		displaySize.x,
		displaySize.y
	);

	if (!bIsEnemyOnScreen) return false;

	float distance = std::hypot(
		screenPos.x - screenCenter.x,
		screenPos.y - screenCenter.y
	);

	return distance <= globals.settings.aimbot.fov;
}

FVector2D Aimbot::getClosestTargetToCursor(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo) {
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    const ImVec2 screenCenter(displaySize.x / 2.0f, displaySize.y / 2.0f);

	FVector2D bestHeadPosition = { 0, 0 };
	float closestDistance = FLT_MAX;

	for (const TrackedActor& actor : actors) {
		if (!this->isInFoV(actor, viewInfo) || actor.isLocalPlayer || actor.sameTeam) continue;

		FVector2D currentEnemyScrenPos;

		bool bHeadPosition2D = this->unreal.WorldToScreen(
			viewInfo,
			actor.location,
			currentEnemyScrenPos,
			displaySize.x,
			displaySize.y
		);

		if (!bHeadPosition2D) continue;

		float distance = std::hypot(
			currentEnemyScrenPos.x - screenCenter.x,
			currentEnemyScrenPos.y - screenCenter.y
		);

		if (distance > closestDistance) continue;

		closestDistance = distance;
		bestHeadPosition = currentEnemyScrenPos;
	}

	return bestHeadPosition;
}

FVector2D Aimbot::applyAimSmoothing(const FVector2D& headPosition2D) {
	POINT currentMousePosition;
	if (!GetCursorPos(&currentMousePosition)) return { 0, 0 };

	FVector2D delta = {
		headPosition2D.x - currentMousePosition.x,
		headPosition2D.y - currentMousePosition.y
	};

	FVector2D step = {
		delta.x / globals.settings.aimbot.smooth,
		delta.y / globals.settings.aimbot.smooth
	};

	return step;
}