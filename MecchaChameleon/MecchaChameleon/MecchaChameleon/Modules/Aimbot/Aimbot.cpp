#include "Aimbot.hpp"
#include <cmath>
#include <Windows.h>

void Aimbot::onAimbot(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo) {
	if (!globals.settings.aimbot.enabled) return;

	if (!(GetAsyncKeyState(globals.settings.aimbot.keybind) & 0x8000)) {
		this->lockedPawn = 0;
		return;
	}

	FVector2D bestActorHeadPosition2D{};
	if (!this->getClosestTargetToCursor(actors, viewInfo, bestActorHeadPosition2D))
		return;

	FVector2D step2D = this->applyAimSmoothing(bestActorHeadPosition2D);

	if (step2D.x == 0 && step2D.y == 0) return;

	INPUT input = {};
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;
	input.mi.dx = static_cast<LONG>(step2D.x);
	input.mi.dy = static_cast<LONG>(step2D.y);
	SendInput(1, &input, sizeof(INPUT));
}

bool Aimbot::isAimTarget(const TrackedActor& actor) {
	return !actor.isLocalPlayer && !actor.sameTeam;
}

bool Aimbot::getActorHeadScreenPos(const TrackedActor& actor, const FMinimalViewInfo& viewInfo, FVector2D& outHeadPosition2D) {
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	FVector headWorld = actor.location + FVector(0, 0, actor.playerSize);

	return this->unreal.WorldToScreen(
		viewInfo,
		headWorld,
		outHeadPosition2D,
		displaySize.x,
		displaySize.y
	);
}

bool Aimbot::isInFoV(const TrackedActor& actor, const FMinimalViewInfo& viewInfo) {
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	const ImVec2 screenCenter(displaySize.x / 2.0f, displaySize.y / 2.0f);
	FVector2D screenPos;

	if (!this->getActorHeadScreenPos(actor, viewInfo, screenPos))
		return false;

	if (!globals.settings.aimbot.fovLimit) return true;

	float distance = std::hypot(
		screenPos.x - screenCenter.x,
		screenPos.y - screenCenter.y
	);

	return distance <= globals.settings.aimbot.fov;
}

bool Aimbot::getClosestTargetToCursor(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo, FVector2D& outHeadPosition2D) {
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	const ImVec2 screenCenter(displaySize.x / 2.0f, displaySize.y / 2.0f);

	if (this->lockedPawn != 0) {
		for (const TrackedActor& actor : actors) {
			if (actor.pawn != this->lockedPawn) continue;

			if (!this->isAimTarget(actor) || !this->isInFoV(actor, viewInfo)) {
				this->lockedPawn = 0;
				break;
			}

			if (this->getActorHeadScreenPos(actor, viewInfo, outHeadPosition2D))
				return true;

			this->lockedPawn = 0;
			break;
		}
	}

	float closestDistance = FLT_MAX;
	uintptr_t bestPawn = 0;
	bool foundTarget = false;

	for (const TrackedActor& actor : actors) {
		if (!this->isAimTarget(actor) || !this->isInFoV(actor, viewInfo)) continue;

		FVector2D currentEnemyScreenPos{};

		if (!this->getActorHeadScreenPos(actor, viewInfo, currentEnemyScreenPos)) continue;

		float distance = std::hypot(
			currentEnemyScreenPos.x - screenCenter.x,
			currentEnemyScreenPos.y - screenCenter.y
		);

		if (distance > closestDistance) continue;

		closestDistance = distance;
		bestPawn = actor.pawn;
		outHeadPosition2D = currentEnemyScreenPos;
		foundTarget = true;
	}

	this->lockedPawn = bestPawn;
	return foundTarget;
}

FVector2D Aimbot::applyAimSmoothing(const FVector2D& headPosition2D) {
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	const FVector2D screenCenter = {
		displaySize.x / 2.0f,
		displaySize.y / 2.0f
	};

	FVector2D delta = {
		headPosition2D.x - screenCenter.x,
		headPosition2D.y - screenCenter.y
	};

	if (std::hypot(delta.x, delta.y) <= 1.f)
		return { 0, 0 };

	float smooth = globals.settings.aimbot.smoothing ? globals.settings.aimbot.smooth : 1.f;

	FVector2D step = {
		delta.x / smooth,
		delta.y / smooth
	};

	return step;
}
