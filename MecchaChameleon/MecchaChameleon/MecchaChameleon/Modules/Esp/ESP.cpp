#include "ESP.hpp"
#include "../../Engine/ImGui/imgui.h"
#include "../../Manager/Globals/Globals.hpp"

#include <iostream>
#include <chrono>

void ESP::renderESP(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo) {
	if (globals.settings.esp.box)
		this->renderBox(actors, viewInfo);
	
	if (globals.settings.esp.name)
		this->renderName(actors, viewInfo);

	if (globals.settings.esp.snaplines)
		this->renderSnaplines(actors, viewInfo);

	if (globals.settings.esp.chineseHat)
		this->renderChineseHat(actors, viewInfo);
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

void ESP::renderName(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo) {
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	FVector2D screenTop;

	for (const TrackedActor& actor : actors) {
		bool bVisibleTop = unreal.WorldToScreen(
			viewInfo,
			actor.location + FVector(0, 0, actor.playerSize),
			screenTop,
			displaySize.x,
			displaySize.y
		);

		if (!bVisibleTop)
			continue;

		ImVec2 textSize = ImGui::CalcTextSize(actor.playerName.c_str());

		ImVec2 textPos(
			screenTop.x - (textSize.x * 0.5f),
			screenTop.y - textSize.y - 4.0f
		);

		drawList->AddText(
			ImVec2(textPos.x + 1.0f, textPos.y + 1.0f),
			IM_COL32(0, 0, 0, 255),
			actor.playerName.c_str()
		);

		drawList->AddText(
			ImVec2(textPos.x - 1.0f, textPos.y - 1.0f),
			IM_COL32(0, 0, 0, 255),
			actor.playerName.c_str()
		);

		drawList->AddText(
			ImVec2(textPos.x - 1.0f, textPos.y + 1.0f),
			IM_COL32(0, 0, 0, 255),
			actor.playerName.c_str()
		);

		drawList->AddText(
			ImVec2(textPos.x + 1.0f, textPos.y - 1.0f),
			IM_COL32(0, 0, 0, 255),
			actor.playerName.c_str()
		);

		drawList->AddText(
			textPos,
			IM_COL32(255, 255, 255, 255),
			actor.playerName.c_str()
		);
	}
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
		if (!unreal.WorldToScreen(viewInfo, FVector(actor.location.x, actor.location.y, actor.location.z - actor.playerSize), screenPos, displaySize.x, displaySize.y)) {
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

// this is still very broken, so not integrated into the ui yet.
void ESP::renderChineseHat(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo) {
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

	constexpr int segments = 64;
	constexpr float twoPi = 6.28318530718f;

	for (const TrackedActor& actor : actors) {
		FVector baseCenter = actor.location + FVector(0, 0, actor.playerSize * 1.3);
		float hatRadius = 35.0f;
		float hatHeight = 15.0f;

		FVector tipWorld = baseCenter + FVector(0, 0, 15);

		FVector2D screenTip;
		if (!unreal.WorldToScreen(viewInfo, tipWorld, screenTip, displaySize.x, displaySize.y))
			continue;

		drawList->AddCircleFilled(ImVec2(screenTip.x, screenTip.y), 4.0f, IM_COL32(255, 0, 0, 255));

		FVector2D lastScreenEdge{};
		bool hasLastEdge = false;

		for (int i = 0; i <= segments; i++) {
			float angle = (static_cast<float>(i) / static_cast<float>(segments)) * twoPi;

			FVector edgeWorld = baseCenter + FVector(
				std::cos(angle) * hatRadius,
				std::sin(angle) * hatRadius,
				0
			);

			FVector2D screenEdge;
			if (!unreal.WorldToScreen(viewInfo, edgeWorld, screenEdge, displaySize.x, displaySize.y)) {
				hasLastEdge = false;
				continue;
			}

			ImColor color = ImColor::HSV(static_cast<float>(i) / segments, 1.0f, 1.0f);

			drawList->AddLine(ImVec2(screenTip.x, screenTip.y), ImVec2(screenEdge.x, screenEdge.y), color, 1.0f);

			if (hasLastEdge)
				drawList->AddLine(ImVec2(lastScreenEdge.x, lastScreenEdge.y), ImVec2(screenEdge.x, screenEdge.y), color, 1.5f);

			lastScreenEdge = screenEdge;
			hasLastEdge = true;
		}
	}
}