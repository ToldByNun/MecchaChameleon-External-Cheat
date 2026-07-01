#include "ESP.hpp"
#include "../../Engine/ImGui/imgui.h"
#include "../../Manager/Globals/Globals.hpp"

#include <algorithm>
#include <cmath>
#include <cfloat>
#include <utility>

bool isRenderValid(bool sameTeam, bool onlyEnemiesEnabled, bool isLocalPlayer, bool devMode) {
	if (sameTeam && onlyEnemiesEnabled) return false;

	if (devMode) return true;

	if (isLocalPlayer) return false;

	return true;
}

// also very scuffed but its on my todo list
ImU32 getESPColor(const TrackedActor& actor, const ImVec4& defaultSetting, const ImVec4& enemySetting) {
	const ImVec4& color = (globals.settings.esp.isTeammateColorEnabled && !actor.sameTeam)
		? enemySetting
		: defaultSetting;
	return Custom::ColorToU32(color);
}

void drawOutlinedLine(ImDrawList* drawList, ImVec2 from, ImVec2 to, ImU32 color) {
	drawList->AddLine(from, to, IM_COL32(0, 0, 0, 255), 4.0f);
	drawList->AddLine(from, to, color, 2.0f);
}

// pretty much just helpers. u can ignore these (except if u want to paste bones kekw)
namespace {
	constexpr int kHeadBone = 6;
	constexpr int kHeadEndBone = 7;

	constexpr std::pair<int, int> skeletonPairs[] = {
		{ 2,  3 }, { 3,  4 }, { 4,  5 }, { 5,  6 },
		{ 5,  8 }, { 8,  9 }, { 9, 10 }, { 10, 11 },
		{ 5, 13 }, { 13, 14 }, { 14, 15 }, { 15, 16 },
		{ 2, 18 }, { 18, 19 }, { 19, 20 }, { 20, 21 },
		{ 2, 23 }, { 23, 24 }, { 24, 25 }, { 25, 26 },
	};

	struct ActorScreenBounds {
		float top = 0.f;
		float bottom = 0.f;
		float left = 0.f;
		float right = 0.f;
		bool valid = false;
	};

	FVector boneToVector(const FVectorD& bone) {
		return FVector{
			static_cast<float>(bone.x),
			static_cast<float>(bone.y),
			static_cast<float>(bone.z)
		};
	}

	bool tryGetActorScreenBounds(
		Unreal& unreal,
		const TrackedActor& actor,
		const FMinimalViewInfo& viewInfo,
		float screenWidth,
		float screenHeight,
		ActorScreenBounds& outBounds
	) {
		if (actor.boneList.size() < kSkeletonBoneCount)
			return false;

		float minX = FLT_MAX;
		float maxX = -FLT_MAX;
		float minY = FLT_MAX;
		float maxY = -FLT_MAX;
		bool anyVisible = false;

		for (const FVectorD& bone : actor.boneList) {
			FVector2D screenPos;
			if (!unreal.WorldToScreen(viewInfo, boneToVector(bone), screenPos, screenWidth, screenHeight))
				continue;

			anyVisible = true;
			minX = min(minX, static_cast<float>(screenPos.x));
			maxX = max(maxX, static_cast<float>(screenPos.x));
			minY = min(minY, static_cast<float>(screenPos.y));
			maxY = max(maxY, static_cast<float>(screenPos.y));
		}

		if (!anyVisible)
			return false;

		constexpr float padding = 3.f;
		outBounds.left = minX - padding;
		outBounds.right = maxX + padding;
		outBounds.top = minY - padding;
		outBounds.bottom = maxY + padding;
		outBounds.valid = true;
		return true;
	}

	bool tryGetLegacyActorScreenBounds(
		Unreal& unreal,
		const TrackedActor& actor,
		const FMinimalViewInfo& viewInfo,
		float screenWidth,
		float screenHeight,
		ActorScreenBounds& outBounds
	) {
		FVector2D screenBottom;
		FVector2D screenTop;

		const bool visibleBottom = unreal.WorldToScreen(
			viewInfo,
			actor.location - FVector(0, 0, actor.playerSize),
			screenBottom,
			screenWidth,
			screenHeight
		);

		const bool visibleTop = unreal.WorldToScreen(
			viewInfo,
			actor.location + FVector(0, 0, actor.playerSize),
			screenTop,
			screenWidth,
			screenHeight
		);

		if (!visibleBottom || !visibleTop)
			return false;

		const float height2D = std::abs(screenTop.y - screenBottom.y);
		const float width2D = height2D * 0.55f;
		const float centerX = screenBottom.x;

		outBounds.top = screenTop.y;
		outBounds.bottom = screenBottom.y;
		outBounds.left = centerX - width2D * 0.5f;
		outBounds.right = centerX + width2D * 0.5f;
		outBounds.valid = true;
		return true;
	}

	bool tryGetActorScreenBoundsForEsp(
		Unreal& unreal,
		const TrackedActor& actor,
		const FMinimalViewInfo& viewInfo,
		float screenWidth,
		float screenHeight,
		bool dynamicBoxes,
		ActorScreenBounds& outBounds
	) {
		if (dynamicBoxes)
			return tryGetActorScreenBounds(unreal, actor, viewInfo, screenWidth, screenHeight, outBounds);

		return tryGetLegacyActorScreenBounds(unreal, actor, viewInfo, screenWidth, screenHeight, outBounds);
	}
}

void ESP::renderESP(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo) {
	if (globals.settings.esp.box)
		this->renderBox(actors, viewInfo);

	if (globals.settings.esp.corners)
		this->renderCorners(actors, viewInfo);

	if (globals.settings.esp.skeleton)
		this->renderSkeleton(actors, viewInfo);
	
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

	for (const TrackedActor& actor : actors) {
		if (!isRenderValid(actor.sameTeam, globals.settings.esp.onlyEnemies, actor.isLocalPlayer, globals.settings.esp.devMode)) continue;

		ActorScreenBounds bounds{};
		if (!tryGetActorScreenBoundsForEsp(
			this->unreal, actor, viewInfo, displaySize.x, displaySize.y,
			globals.settings.esp.dynamicBoxes, bounds
		))
			continue;

		const ImVec2 topLeft(bounds.left, bounds.top);
		const ImVec2 bottomRight(bounds.right, bounds.bottom);

		drawList->AddRect(
			topLeft,
			bottomRight,
			IM_COL32(0, 0, 0, 255),
			0.0f, ImDrawFlags_None, 4.0f
		);

		drawList->AddRect(
			topLeft,
			bottomRight,
			getESPColor(actor, globals.settings.esp.defaultBoxColor, globals.settings.esp.enemyBoxColor),
			0.0f, ImDrawFlags_None, 2.0f
		);
	}
}

void ESP::renderCorners(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo) {
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

	for (const TrackedActor& actor : actors) {
		if (!isRenderValid(actor.sameTeam, globals.settings.esp.onlyEnemies, actor.isLocalPlayer, globals.settings.esp.devMode)) continue;

		ActorScreenBounds bounds{};
		if (!tryGetActorScreenBoundsForEsp(
			this->unreal, actor, viewInfo, displaySize.x, displaySize.y,
			globals.settings.esp.dynamicBoxes, bounds
		))
			continue;

		const float left = bounds.left;
		const float right = bounds.right;
		const float top = bounds.top;
		const float bottom = bounds.bottom;
		const float width2D = right - left;
		const float height2D = bottom - top;

		const float lineWidth = width2D * 0.25f;
		const float lineHeight = height2D * 0.20f;

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
				getESPColor(actor, globals.settings.esp.defaultBoxColor, globals.settings.esp.enemyBoxColor),
				2.0f
			);
			drawList->AddLine(
				ImVec2(x, y),
				ImVec2(x, y + (directionY * lineHeight)),
				getESPColor(actor, globals.settings.esp.defaultBoxColor, globals.settings.esp.enemyBoxColor),
				2.0f
			);
		};

		drawCorner(left, top, 1.0f, 1.0f);
		drawCorner(right, top, -1.0f, 1.0f);
		drawCorner(left, bottom, 1.0f, -1.0f);
		drawCorner(right, bottom, -1.0f, -1.0f);
	}
}

void ESP::renderSkeleton(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo) {
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

	for (const TrackedActor& actor : actors) {
		if (!isRenderValid(actor.sameTeam, globals.settings.esp.onlyEnemies, actor.isLocalPlayer, globals.settings.esp.devMode)) continue;
		if (actor.boneList.size() < kSkeletonBoneCount) continue;

		auto getBoneScreen = [&](int index, FVector2D& out) -> bool {
			const FVectorD& bone = actor.boneList[index];

			return this->unreal.WorldToScreen(
				viewInfo,
				FVector{ bone.x, bone.y, bone.z },
				out,
				displaySize.x,
				displaySize.y
			);
		};

		for (const std::pair<int, int>& pair : skeletonPairs) {
			FVector2D from, to;

			if (!getBoneScreen(pair.first, from) || !getBoneScreen(pair.second, to)) continue;

			const float dx = to.x - from.x;
			const float dy = to.y - from.y;
			if ((dx * dx + dy * dy) < 4.0f) continue;

			drawList->AddLine(
				ImVec2(from.x, from.y),
				ImVec2(to.x, to.y),
				getESPColor(actor, globals.settings.esp.defaultSkeletonColor, globals.settings.esp.enemySkeletonColor),
				1.0f
			);
		}
	}
}

void ESP::renderNameDistance(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo, LabelType type) {
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	FVector2D screenTop;

	for (const TrackedActor& actor : actors) {
		if (!isRenderValid(actor.sameTeam, globals.settings.esp.onlyEnemies, actor.isLocalPlayer, globals.settings.esp.devMode)) continue;

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
		if (!isRenderValid(actor.sameTeam, globals.settings.esp.onlyEnemies, actor.isLocalPlayer, globals.settings.esp.devMode)) continue;

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
			getESPColor(actor, globals.settings.esp.defaultSnaplineColor, globals.settings.esp.enemySnaplineColor),
			2.0f
		);

		linesDrawn++;
	}
}

void ESP::renderChineseHat(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo) {
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

	constexpr int segments = 64;
	constexpr float twoPi = 6.28318530718f;

	for (const TrackedActor& actor : actors) {
		if (!isRenderValid(actor.sameTeam, globals.settings.esp.onlyEnemies, actor.isLocalPlayer, globals.settings.esp.devMode)) continue;
		if (actor.boneList.size() < kSkeletonBoneCount) continue;

		const FVector headWorld = boneToVector(actor.boneList[kHeadBone]);
		const float hatRadius = actor.headRadius > 1.0 ? static_cast<float>(actor.headRadius * 1.1) : 30.f;
		const float hatHeight = actor.headRadius > 1.0 ? static_cast<float>(actor.headRadius * 0.55) : 15.f;
		const float hatVerticalOffset = actor.headRadius > 1.0 ? static_cast<float>(actor.headRadius * 0.45) : 14.f;

		FVector hatUp{ 0.f, 0.f, 1.f };
		if (actor.boneList.size() > kHeadEndBone) {
			const FVector headEndWorld = boneToVector(actor.boneList[kHeadEndBone]);
			const FVector delta = headEndWorld - headWorld;
			const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);

			if (length > 1.f) {
				hatUp = FVector(delta.x / length, delta.y / length, delta.z / length);
			}
		}

		const FVector hatOrigin = headWorld + FVector(
			hatUp.x * hatVerticalOffset,
			hatUp.y * hatVerticalOffset,
			hatUp.z * hatVerticalOffset
		);
		const FVector baseCenter = hatOrigin;
		const FVector tipWorld = hatOrigin + FVector(hatUp.x * hatHeight, hatUp.y * hatHeight, hatUp.z * hatHeight);

		FVector2D screenTip;
		if (!unreal.WorldToScreen(viewInfo, tipWorld, screenTip, displaySize.x, displaySize.y)) continue;

		drawList->AddCircleFilled(ImVec2(screenTip.x, screenTip.y), 4.0f, IM_COL32(255, 0, 0, 255));

		FVector brimRight = FVector(1.f, 0.f, 0.f);
		if (std::abs(hatUp.z) > 0.95f) {
			brimRight = FVector(0.f, 1.f, 0.f);
		}

		FVector brimForward = FVector(
			hatUp.y * brimRight.z - hatUp.z * brimRight.y,
			hatUp.z * brimRight.x - hatUp.x * brimRight.z,
			hatUp.x * brimRight.y - hatUp.y * brimRight.x
		);

		const float brimForwardLength = std::sqrt(
			brimForward.x * brimForward.x +
			brimForward.y * brimForward.y +
			brimForward.z * brimForward.z
		);

		if (brimForwardLength <= 0.001f) continue;

		brimForward = FVector(
			brimForward.x / brimForwardLength,
			brimForward.y / brimForwardLength,
			brimForward.z / brimForwardLength
		);

		brimRight = FVector(
			brimForward.y * hatUp.z - brimForward.z * hatUp.y,
			brimForward.z * hatUp.x - brimForward.x * hatUp.z,
			brimForward.x * hatUp.y - brimForward.y * hatUp.x
		);

		FVector2D lastScreenEdge{};
		bool hasLastEdge = false;

		for (int i = 0; i <= segments; i++) {
			const float angle = (static_cast<float>(i) / static_cast<float>(segments)) * twoPi;
			const float cosAngle = std::cos(angle);
			const float sinAngle = std::sin(angle);

			const FVector edgeWorld = baseCenter + FVector(
				(brimRight.x * cosAngle + brimForward.x * sinAngle) * hatRadius,
				(brimRight.y * cosAngle + brimForward.y * sinAngle) * hatRadius,
				(brimRight.z * cosAngle + brimForward.z * sinAngle) * hatRadius
			);

			FVector2D screenEdge;
			if (!this->unreal.WorldToScreen(viewInfo, edgeWorld, screenEdge, displaySize.x, displaySize.y)) {
				hasLastEdge = false;
				continue;
			}

			const ImColor color = ImColor::HSV(static_cast<float>(i) / segments, 1.0f, 1.0f);

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