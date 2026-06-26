#include "Slider.hpp"

#include "../imgui_internal.h"

#include <cstdio>

namespace Custom {

namespace {
	float Clamp(float v, float lo, float hi) {
		if (v < lo) return lo;
		if (v > hi) return hi;
		return v;
	}
}

bool SliderFloat(
	const char* label,
	float* value,
	float min,
	float max,
	const char* fmt,
	const SliderPreset& preset
) {
	if (!value)
		return false;

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	const ImGuiID id = window->GetID(label);
	const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);

	char valueText[64];
	snprintf(valueText, sizeof(valueText), fmt, *value);
	const ImVec2 valueSize = ImGui::CalcTextSize(valueText);

	const float availWidth = ImGui::GetContentRegionAvail().x;
	const float trackWidth = availWidth - labelSize.x - valueSize.x - preset.spacing * 2.f;
	const float rowHeight = preset.rowHeight;

	const ImVec2 pos = window->DC.CursorPos;
	const ImRect totalBb(pos, ImVec2(pos.x + availWidth, pos.y + rowHeight));
	ImGui::ItemSize(totalBb);
	if (!ImGui::ItemAdd(totalBb, id))
		return false;

	const float trackX = pos.x + labelSize.x + preset.spacing;
	const float trackY = pos.y + (rowHeight - preset.height) * 0.5f;
	const ImRect trackBb(
		ImVec2(trackX, trackY),
		ImVec2(trackX + trackWidth, trackY + preset.height)
	);

	bool hovered = false;
	bool held = false;
	ImGui::ButtonBehavior(trackBb, id, &hovered, &held);

	if (held || hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		const float mouseX = ImGui::GetIO().MousePos.x;
		const float t = Clamp((mouseX - trackBb.Min.x) / trackBb.GetWidth(), 0.f, 1.f);
		*value = min + (max - min) * t;
	}

	const float t = (max > min) ? Clamp((*value - min) / (max - min), 0.f, 1.f) : 0.f;
	const float grabX = trackBb.Min.x + trackBb.GetWidth() * t;

	ImDrawList* drawList = window->DrawList;

	const float labelY = pos.y + (rowHeight - labelSize.y) * 0.5f;
	drawList->AddText(ImVec2(pos.x, labelY), ColorToU32(preset.labelColor), label);

	drawList->AddRectFilled(
		trackBb.Min,
		trackBb.Max,
		ColorToU32(preset.trackColor),
		preset.rounding
	);

	if (t > 0.f) {
		drawList->AddRectFilled(
			trackBb.Min,
			ImVec2(grabX, trackBb.Max.y),
			ColorToU32(preset.fillColor),
			preset.rounding
		);
	}

	drawList->AddCircleFilled(
		ImVec2(grabX, trackBb.Min.y + preset.height * 0.5f),
		preset.grabRadius,
		ColorToU32(preset.grabColor)
	);

	const float valueY = pos.y + (rowHeight - valueSize.y) * 0.5f;
	drawList->AddText(
		ImVec2(trackBb.Max.x + preset.spacing, valueY),
		ColorToU32(preset.valueColor),
		valueText
	);

	return held;
}

} // namespace Custom
