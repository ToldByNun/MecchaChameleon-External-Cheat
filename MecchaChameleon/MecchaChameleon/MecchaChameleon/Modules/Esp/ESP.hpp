#ifndef ESP_HPP
#define ESP_HPP

#include "../../Manager/Classmanager/Classmanager.hpp"
#include "../../Engine/types.hpp"
#include "../../Engine/MecchaChameleon/MecchaChameleon.hpp"
#include "../../Engine/Unreal/Unreal.hpp"
#include <vector>

enum LabelType {
	NAME,
	DISTANCE,
	NAMEDISTANCE
};

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

};

#endif // ESP_HPP
