#include "ESP.hpp"
#include "../../Engine/ImGui/imgui.h"
#include "../../Manager/Globals/Globals.hpp"

#include <iostream>
#include <chrono>
#include <vector>

bool isRenderValid(bool sameTeam, bool onlyEnemiesEnabled, bool isLocalPlayer) {
	if (sameTeam && onlyEnemiesEnabled) return false;

	if (isLocalPlayer) return false;

	return true;
}

ImU32 getESPColor(const TrackedActor& actor) {
	if (globals.settings.esp.isTeammateColorEnabled) {
		return actor.sameTeam ? IM_COL32(255, 255, 255, 255) : IM_COL32(237, 52, 52, 255);
	}

	return IM_COL32(255, 255, 255, 255);
}

void ESP::renderESP(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo) {
	if (globals.settings.esp.box)
		this->renderBox(actors, viewInfo);

	if (globals.settings.esp.corners)
		this->renderCorners(actors, viewInfo);
	
	if (globals.settings.esp.name && globals.settings.esp.distance)
		this->renderNameDistance(actors, viewInfo, LabelType::NAMEDISTANCE);
	else if (globals.settings.esp.name)
		this->renderNameDistance(actors, viewInfo, LabelType::NAME);
	else if (globals.settings.esp.distance)
		this->renderNameDistance(actors, viewInfo, LabelType::DISTANCE);

	if (globals.settings.esp.snaplines)
		this->renderSnaplines(actors, viewInfo);

	if (globals.settings.esp.chineseHat)
		this->renderChineseHat(actors, viewInfo);

	if (globals.settings.esp.fovCircle)
		this->renderFoV(globals.settings.aimbot.fov);
}

void ESP::renderBox(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo) {
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	FVector2D screenBottom, screenTop;

	for (const TrackedActor& actor : actors) {
		if (!isRenderValid(actor.sameTeam, globals.settings.esp.onlyEnemies, actor.isLocalPlayer)) continue;

		bool bVisibleBottom = this->unreal.WorldToScreen(
			viewInfo,
			actor.location - FVector(0, 0, actor.playerSize),
			screenBottom,
			displaySize.x,
			displaySize.y
		);

		bool bVisibleTop = this->unreal.WorldToScreen(
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
			ImVec2(topLeft.x, topLeft.y),
			ImVec2(bottomRight.x, bottomRight.y),
			IM_COL32(0, 0, 0, 255),
			0.0f, ImDrawFlags_None, 4.0f
		);

		// very scuffed but to simplify it:
		// if isTeammateColorEnabled true and if they are on the same team, then use box color
		// if they arent on the same team enemyboxcolor
		// and if isTeammateColorEnabled false, then use default (boxColor)

		drawList->AddRect(
			topLeft,
			bottomRight,
			getESPColor(actor),
			0.0f, ImDrawFlags_None, 2.0f
		);
	}
}

void ESP::renderCorners(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo) {
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	FVector2D screenBottom, screenTop;

	for (const TrackedActor& actor : actors) {
		if (!isRenderValid(actor.sameTeam, globals.settings.esp.onlyEnemies, actor.isLocalPlayer)) continue;

		bool bVisibleBottom = this->unreal.WorldToScreen(
			viewInfo,
			actor.location - FVector(0, 0, actor.playerSize),
			screenBottom,
			displaySize.x,
			displaySize.y
		);

		bool bVisibleTop = this->unreal.WorldToScreen(
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

		float left = centerX - (width2D * 0.5f);
		float right = centerX + (width2D * 0.5f);
		float top = screenTop.y;
		float bottom = screenBottom.y;

		float lineWidth = width2D * 0.25f;
		float lineHeight = height2D * 0.20f;

		auto drawCorner = [&](float x, float y, float directionX, float directionY) {
			drawList->AddLine(
				ImVec2(x, y),
				ImVec2(x + (directionX * lineWidth), y),
				IM_COL32(0, 0, 0, 255),
				4.0f
			);
			drawList->AddLine(
				ImVec2(x, y),
				ImVec2(x, y + (directionY * lineHeight)),
				IM_COL32(0, 0, 0, 255),
				4.0f
			);

			drawList->AddLine(
				ImVec2(x, y),
				ImVec2(x + (directionX * lineWidth), y),
				getESPColor(actor),
				2.0f
			);
			drawList->AddLine(
				ImVec2(x, y),
				ImVec2(x, y + (directionY * lineHeight)),
				getESPColor(actor),
				2.0f
			);
		};

		drawCorner(left, top, 1.0f, 1.0f);
		drawCorner(right, top, -1.0f, 1.0f);
		drawCorner(left, bottom, 1.0f, -1.0f);
		drawCorner(right, bottom, -1.0f, -1.0f);
	}
}

void ESP::renderNameDistance(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo, LabelType type) {
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	FVector2D screenTop;

	for (const TrackedActor& actor : actors) {
		if (!isRenderValid(actor.sameTeam, globals.settings.esp.onlyEnemies, actor.isLocalPlayer)) continue;

		bool bVisibleTop = this->unreal.WorldToScreen(
			viewInfo,
			actor.location + FVector(0, 0, actor.playerSize),
			screenTop,
			displaySize.x,
			displaySize.y
		);

		if (!bVisibleTop) continue;

		std::string text;

		if (type == LabelType::NAME) {
			text = actor.playerName;
		}
		else if (type == LabelType::DISTANCE) {
			FVector distanceFrom = viewInfo.Location;

			for (const TrackedActor& localActor : actors) {
				if (localActor.isLocalPlayer) {
					distanceFrom = localActor.location;
					break;
				}
			}

			double dx = actor.location.x - distanceFrom.x;
			double dy = actor.location.y - distanceFrom.y;
			double dz = actor.location.z - distanceFrom.z;

			double distance = std::sqrt(dx * dx + dy * dy + dz * dz) / 100.0;

			text = std::to_string(static_cast<int>(distance)) + "m";
		}
		else if (type == LabelType::NAMEDISTANCE) {
			FVector distanceFrom = viewInfo.Location;

			for (const TrackedActor& localActor : actors) {
				if (localActor.isLocalPlayer) {
					distanceFrom = localActor.location;
					break;
				}
			}

			double dx = actor.location.x - distanceFrom.x;
			double dy = actor.location.y - distanceFrom.y;
			double dz = actor.location.z - distanceFrom.z;

			double distance = std::sqrt(dx * dx + dy * dy + dz * dz) / 100.0;

			text = actor.playerName + " / " + std::to_string(static_cast<int>(distance)) + "m";
		}

		if (text.empty()) continue;

		ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
		ImVec2 textPos = ImVec2(
			screenTop.x - (textSize.x * 0.5f),
			screenTop.y - textSize.y - 4.0f
		);

		drawList->AddText(ImVec2(textPos.x + 1.0f, textPos.y + 1.0f), IM_COL32(0, 0, 0, 255), text.c_str());
		drawList->AddText(ImVec2(textPos.x - 1.0f, textPos.y - 1.0f), IM_COL32(0, 0, 0, 255), text.c_str());
		drawList->AddText(ImVec2(textPos.x - 1.0f, textPos.y + 1.0f), IM_COL32(0, 0, 0, 255), text.c_str());
		drawList->AddText(ImVec2(textPos.x + 1.0f, textPos.y - 1.0f), IM_COL32(0, 0, 0, 255), text.c_str());

		drawList->AddText(
			textPos,
			IM_COL32(255, 255, 255, 255),
			text.c_str()
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
		if (!isRenderValid(actor.sameTeam, globals.settings.esp.onlyEnemies, actor.isLocalPlayer)) continue;

		FVector2D screenPos;
		if (!this->unreal.WorldToScreen(viewInfo, FVector(actor.location.x, actor.location.y, actor.location.z - actor.playerSize), screenPos, displaySize.x, displaySize.y)) {
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
			IM_COL32(0, 0, 0, 255),
			4.0f
		);

		drawList->AddLine(
			screenBottom,
			ImVec2((float)screenPos.x, (float)screenPos.y),
			getESPColor(actor),
			2.0f
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
		if (!isRenderValid(actor.sameTeam, globals.settings.esp.onlyEnemies, actor.isLocalPlayer)) continue;

		FVector baseCenter = actor.location + FVector(0, 0, actor.playerSize * 0.8f);
		float hatRadius = 35.0f;
		float hatHeight = 15.0f;

		FVector tipWorld = baseCenter + FVector(0, 0, 15);

		FVector2D screenTip;
		if (!unreal.WorldToScreen(viewInfo, tipWorld, screenTip, displaySize.x, displaySize.y)) continue;

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
			if (!this->unreal.WorldToScreen(viewInfo, edgeWorld, screenEdge, displaySize.x, displaySize.y)) {
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

void ESP::renderFoV(float fov) {
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

	const ImVec2 displayMiddle = ImVec2(displaySize.x / 2, displaySize.y / 2);

	drawList->AddCircle(displayMiddle, fov, ImColor(0, 0, 0, 255), 64, 3.0f);
	drawList->AddCircle(displayMiddle, fov, ImColor(255, 255, 255, 255), 64, 1.0f);
}