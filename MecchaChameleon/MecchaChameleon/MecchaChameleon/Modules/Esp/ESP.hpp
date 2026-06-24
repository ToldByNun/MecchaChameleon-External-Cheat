#ifndef ESP_HPP
#define ESP_HPP

#include "../../Manager/Classmanager/Classmanager.hpp"
#include "../../Engine/types.hpp"
#include "../../Engine/MecchaChameleon/MecchaChameleon.hpp"
#include <vector>

class ESP : public IManagedClass {
public:
	void renderSnaplines(const std::vector<TrackedActor>& actors, const FMinimalViewInfo& viewInfo);
};

#endif // ESP_HPP
