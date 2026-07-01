#ifndef ESP_HPP
#define ESP_HPP

#include "../../Manager/Classmanager/Classmanager.hpp"
#include "../../Manager/Globals/Globals.hpp"
#include "../../Engine/types.hpp"
#include "../../Engine/MecchaChameleon/MecchaChameleon.hpp"
#include "../../Engine/Unreal/Unreal.hpp"
#include <vector>

enum LabelType {
	NAME,
	DISTANCE,
	NAMEDISTANCE
};

// pretty much just helpers. u can ignore these (except if u want to paste bones kekw)
// in hpp because boilerplate
namespace {
	constexpr std::pair<int, int> skeletonPairs[] = {
		{ SPINE1, SPINE2 }, { SPINE2, SPINE3 },
		{ SPINE3, NECK }, { NECK, HEAD },
		{ NECK, SHOULDER_L }, { SHOULDER_L, UPPER_ARM_L },
		{ UPPER_ARM_L, LOWER_ARM_L }, { LOWER_ARM_L, HAND_L },
		{ NECK, SHOULDER_R }, { SHOULDER_R, UPPER_ARM_R },
		{ UPPER_ARM_R, LOWER_ARM_R }, { LOWER_ARM_R, HAND_R },
		{ SPINE1, HIP_L }, { HIP_L, UPPER_LEG_L },
		{ UPPER_LEG_L, LOWER_LEG_L }, { LOWER_LEG_L, FOOT_L },
		{ SPINE1, HIP_R }, { HIP_R, UPPER_LEG_R },
		{ UPPER_LEG_R, LOWER_LEG_R }, { LOWER_LEG_R, FOOT_R },
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

	// MM
	constexpr float kPi = 3.14159265358979f;
	constexpr float kMinimapSize = 160.f;
	constexpr float kMinimapPadding = 14.f;
	constexpr float kMinimapWorldRange = 12000.f;

	bool shouldShowOnMinimap(const TrackedActor& actor) {
		if (actor.isLocalPlayer)
			return false;
		if (actor.sameTeam && globals.settings.esp.hideTeammatesMM)
			return false;
		if (!actor.sameTeam && globals.settings.esp.hideEnemiesMM)
			return false;

		return true;
	}

	ImU32 getMinimapActorColor(const TrackedActor& actor) {
		const ImVec4& color = actor.sameTeam
			? globals.settings.esp.defaultMMColor
			: globals.settings.esp.enemyMMColor;

		return Custom::ColorToU32(color);
	}

	bool tryGetLocalPlayer(const std::vector<TrackedActor>& actors, FVector& outLocation) {
		for (const TrackedActor& actor : actors) {
			if (actor.isLocalPlayer) {
				outLocation = actor.location;
				return true;
			}
		}

		return false;
	}

	float getActorFacingRadians(const TrackedActor& actor) {
		if (actor.boneList.size() > HEAD_END) {
			const double dx = actor.boneList[HEAD_END].x - actor.boneList[HEAD].x;
			const double dy = actor.boneList[HEAD_END].y - actor.boneList[HEAD].y;

			if (std::abs(dx) > 0.01 || std::abs(dy) > 0.01)
				return static_cast<float>(std::atan2(dy, dx));
		}

		return 0.f;
	}

	ImVec2 worldDeltaToMinimap(
		double deltaX,
		double deltaY,
		float playerYawRadians,
		const ImVec2& center,
		float scale
	) {
		const float cosYaw = std::cos(-playerYawRadians);
		const float sinYaw = std::sin(-playerYawRadians);
		const float rotatedX = static_cast<float>(deltaX) * cosYaw - static_cast<float>(deltaY) * sinYaw;
		const float rotatedY = static_cast<float>(deltaX) * sinYaw + static_cast<float>(deltaY) * cosYaw;

		return ImVec2(
			center.x + rotatedX * scale,
			center.y - rotatedY * scale
		);
	}

	void drawDirectionArrow(
		ImDrawList* drawList,
		const ImVec2& position,
		float directionRadians,
		float playerYawRadians,
		ImU32 color
	) {
		const float arrowAngle = directionRadians - playerYawRadians - (kPi * 0.5f);
		const float arrowSize = 5.f;

		const ImVec2 tip(
			position.x + std::cos(arrowAngle) * arrowSize,
			position.y + std::sin(arrowAngle) * arrowSize
		);
		const ImVec2 left(
			position.x + std::cos(arrowAngle + 2.4f) * arrowSize * 0.55f,
			position.y + std::sin(arrowAngle + 2.4f) * arrowSize * 0.55f
		);
		const ImVec2 right(
			position.x + std::cos(arrowAngle - 2.4f) * arrowSize * 0.55f,
			position.y + std::sin(arrowAngle - 2.4f) * arrowSize * 0.55f
		);

		drawList->AddTriangleFilled(tip, left, right, color);
	}
}

class ESP : public IManagedClass {
public:
	Unreal unreal;

	void renderESP(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo);
	void renderBox(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo);
	void renderCorners(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo);
	void renderSkeleton(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo);
	void renderNameDistance(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo, LabelType type);
	void renderSnaplines(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo);
	void renderChineseHat(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo);
	void renderFoV(float fov);
	void renderMinimap(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo);

};

#endif // ESP_HPP
