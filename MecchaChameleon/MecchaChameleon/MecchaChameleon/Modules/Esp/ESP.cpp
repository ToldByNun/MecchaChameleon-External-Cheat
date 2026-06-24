#include "ESP.hpp"
#include "../../Engine/Unreal/Unreal.hpp"
#include "../../Engine/ImGui/imgui.h"

#include <iostream>
#include <chrono>

void ESP::renderSnaplines(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo) {
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	const ImVec2 screenBottom = { displaySize.x * 0.5f, displaySize.y };

	if (actors.empty()) return;

	Unreal unreal;
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
