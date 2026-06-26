#include "Toggle.hpp"

#include "../imgui_internal.h"

namespace Custom {

bool Toggle(const char* label, bool* value, const TogglePreset& preset) {
	if (!value)
		return false;

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	const ImGuiID id = window->GetID(label);
	const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);
	const float rowHeight = ImMax(preset.boxSize, labelSize.y);
	const float totalWidth = preset.boxSize + preset.spacing + labelSize.x;

	const ImVec2 pos = window->DC.CursorPos;
	const ImRect totalBb(pos, ImVec2(pos.x + totalWidth, pos.y + rowHeight));
	ImGui::ItemSize(totalBb);
	if (!ImGui::ItemAdd(totalBb, id))
		return false;

	bool hovered = false;
	bool held = false;
	const bool pressed = ImGui::ButtonBehavior(totalBb, id, &hovered, &held);
	if (pressed)
		*value = !*value;

	const ImRect boxBb(
		pos,
		ImVec2(pos.x + preset.boxSize, pos.y + preset.boxSize)
	);
	const float boxOffsetY = (rowHeight - preset.boxSize) * 0.5f;
	const ImVec2 boxMin(boxBb.Min.x, boxBb.Min.y + boxOffsetY);
	const ImVec2 boxMax(boxBb.Max.x, boxBb.Min.y + boxOffsetY + preset.boxSize);

	ImDrawList* drawList = window->DrawList;
	const ImVec4 borderColor = hovered ? preset.boxBorderHovered : preset.boxBorder;
	drawList->AddRect(
		boxMin,
		boxMax,
		ColorToU32(borderColor),
		preset.rounding,
		0,
		1.f
	);

	if (*value) {
		const float inset = preset.fillInset;
		const float innerSize = preset.boxSize - inset * 2.f;
		const float fillRounding = ImMin(preset.rounding, innerSize * 0.5f - 0.01f);
		drawList->AddRectFilled(
			ImVec2(boxMin.x + inset, boxMin.y + inset),
			ImVec2(boxMax.x - inset, boxMax.y - inset),
			ColorToU32(preset.checkColor),
			fillRounding
		);
	}

	const ImVec4 labelColor = *value ? preset.labelActive : preset.labelInactive;
	const float labelY = pos.y + (rowHeight - labelSize.y) * 0.5f;
	drawList->AddText(
		ImVec2(boxMax.x + preset.spacing, labelY),
		ColorToU32(labelColor),
		label
	);

	return pressed;
}

} // namespace Custom
