#include "ESP.hpp"
#include "../../Engine/ImGui/imgui.h"
#include "../../Manager/Globals/Globals.hpp"

#include <iostream>
#include <chrono>

void ESP::renderESP(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo) {
	if (globals.settings.esp.snaplines)
		this->renderSnaplines(actors, viewInfo);

	if (globals.settings.esp.box)
		this->renderBox(actors, viewInfo);
}

void ESP::renderSnaplines(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo) {
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	const ImVec2 screenBottom = { displaySize.x * 0.5f, displaySize.y };

	if (actors.empty()) return;

	int behindCamera = 0;
	int outOfBounds = 0;
	int linesDrawn = 0;

	for (const TrackedActor& actor : actors) {
		FVector2D screenPos;
		if (!unreal.WorldToScreen(viewInfo, actor.location, screenPos, displaySize.x, displaySize.y)) {
			behindCamera++;
			continue;
		}

		if (screenPos.x < 0 || screenPos.y < 0 || screenPos.x > displaySize.x || screenPos.y > displaySize.y) {
			outOfBounds++;
			continue;
		}

		drawList->AddLine(
			screenBottom,
			ImVec2((float)screenPos.x, (float)screenPos.y),
			IM_COL32(255, 255, 255, 200),
			1.5f
		);

		linesDrawn++;
	}
}

void ESP::renderBox(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo) {
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	FVector2D screenBottom, screenTop;

	for (const TrackedActor& actor : actors) {
		bool bVisibleBottom = unreal.WorldToScreen(
			viewInfo,
			actor.location - FVector(0, 0, actor.playerSize),
			screenBottom,
			displaySize.x,
			displaySize.y
		);

		bool bVisibleTop = unreal.WorldToScreen(
			viewInfo,
			actor.location + FVector(0, 0, actor.playerSize),
			screenTop,
			displaySize.x,
			displaySize.y
		);
		
		if (!bVisibleBottom || !bVisibleTop) continue;

		float height2D = abs(screenTop.y - screenBottom.y);
		float width2D = height2D * 0.55f;
		float centerX = screenBottom.x;

		ImVec2 topLeft = ImVec2(centerX - width2D * 0.5f, screenTop.y);
		ImVec2 bottomRight = ImVec2(centerX + width2D * 0.5f, screenBottom.y);

		drawList->AddRect(
			ImVec2(topLeft.x - 1.0f, topLeft.y - 1.0f),
			ImVec2(bottomRight.x + 1.0f, bottomRight.y + 1.0f),
			IM_COL32(0, 0, 0, 255),
			0.0f, ImDrawFlags_None, 1.0f
		);

		drawList->AddRect(
			topLeft,
			bottomRight,
			IM_COL32(255, 255, 255, 255),
			0.0f, ImDrawFlags_None, 2.0f
		);

		drawList->AddRect(
			ImVec2(topLeft.x + 1.0f, topLeft.y + 1.0f),
			ImVec2(bottomRight.x - 1.0f, bottomRight.y - 1.0f),
			IM_COL32(0, 0, 0, 255),
			0.0f, ImDrawFlags_None, 1.0f
		);
	}
}