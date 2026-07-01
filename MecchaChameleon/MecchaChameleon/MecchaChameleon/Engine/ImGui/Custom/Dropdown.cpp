#include "Dropdown.hpp"

#include "../imgui_internal.h"

#include <cfloat>

namespace Custom {

namespace {
	void DrawArrow(ImDrawList* drawList, const ImVec2& center, float size, ImU32 color) {
		const ImVec2 points[3] = {
			ImVec2(center.x - size * 0.5f, center.y - size * 0.2f),
			ImVec2(center.x + size * 0.5f, center.y - size * 0.2f),
			ImVec2(center.x, center.y + size * 0.35f)
		};

		drawList->AddTriangleFilled(points[0], points[1], points[2], color);
	}
}

bool Dropdown(
	const char* label,
	int* currentIndex,
	const char* const* items,
	int itemCount,
	const DropdownPreset& preset
) {
	if (!currentIndex || !items || itemCount <= 0)
		return false;

	if (*currentIndex < 0)
		*currentIndex = 0;
	if (*currentIndex >= itemCount)
		*currentIndex = itemCount - 1;

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	const ImGuiID id = window->GetID(label);
	const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);
	const char* selectedLabel = items[*currentIndex] ? items[*currentIndex] : "";
	const ImVec2 selectedSize = ImGui::CalcTextSize(selectedLabel, nullptr, true);

	const float availWidth = ImGui::GetContentRegionAvail().x;
	const float boxWidth = availWidth - labelSize.x - preset.spacing;
	const float rowHeight = preset.rowHeight;

	const ImVec2 pos = window->DC.CursorPos;
	const ImRect totalBb(pos, ImVec2(pos.x + availWidth, pos.y + rowHeight));
	ImGui::ItemSize(totalBb);
	if (!ImGui::ItemAdd(totalBb, id))
		return false;

	const float boxX = pos.x + labelSize.x + preset.spacing;
	const float boxY = pos.y + (rowHeight - preset.boxHeight) * 0.5f;
	const ImRect boxBb(
		ImVec2(boxX, boxY),
		ImVec2(boxX + boxWidth, boxY + preset.boxHeight)
	);

	bool hovered = false;
	bool held = false;
	const bool pressed = ImGui::ButtonBehavior(boxBb, id, &hovered, &held);

	ImDrawList* drawList = window->DrawList;

	const float labelY = pos.y + (rowHeight - labelSize.y) * 0.5f;
	drawList->AddText(ImVec2(pos.x, labelY), ColorToU32(preset.labelColor), label);

	bool popupOpen = false;
	ImGui::PushID(id);
	if (pressed)
		ImGui::OpenPopup("##dropdown_popup");
	popupOpen = ImGui::IsPopupOpen("##dropdown_popup", ImGuiPopupFlags_None);

	const ImVec4 borderColor = popupOpen
		? preset.boxBorderOpen
		: (hovered ? preset.boxBorderHovered : preset.boxBorder);

	drawList->AddRectFilled(
		boxBb.Min,
		boxBb.Max,
		ColorToU32(preset.boxBackground),
		preset.rounding
	);
	drawList->AddRect(
		boxBb.Min,
		boxBb.Max,
		ColorToU32(borderColor),
		preset.rounding,
		0,
		1.f
	);

	const float textPadding = 8.f;
	const float textY = boxBb.Min.y + (preset.boxHeight - selectedSize.y) * 0.5f;
	const float textMaxX = boxBb.Max.x - preset.arrowWidth;
	drawList->PushClipRect(
		ImVec2(boxBb.Min.x + textPadding, boxBb.Min.y),
		ImVec2(textMaxX, boxBb.Max.y),
		true
	);
	drawList->AddText(
		ImVec2(boxBb.Min.x + textPadding, textY),
		ColorToU32(preset.textSelected),
		selectedLabel
	);
	drawList->PopClipRect();

	const ImVec2 arrowCenter(
		boxBb.Max.x - preset.arrowWidth * 0.5f,
		boxBb.Min.y + preset.boxHeight * 0.5f
	);
	DrawArrow(drawList, arrowCenter, 8.f, ColorToU32(preset.arrowColor));

	bool changed = false;

	ImGui::SetNextWindowPos(ImVec2(boxBb.Min.x, boxBb.Max.y + 2.f), ImGuiCond_Appearing);
	ImGui::SetNextWindowSizeConstraints(
		ImVec2(boxBb.GetWidth(), 0.f),
		ImVec2(boxBb.GetWidth(), FLT_MAX)
	);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, preset.popupRounding);
	ImGui::PushStyleColor(ImGuiCol_PopupBg, preset.popupBackground);
	ImGui::PushStyleColor(ImGuiCol_Border, preset.popupBorder);

	if (ImGui::BeginPopup("##dropdown_popup", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGuiWindow* popupWindow = ImGui::GetCurrentWindow();
		ImDrawList* popupDrawList = popupWindow->DrawList;

		for (int itemIndex = 0; itemIndex < itemCount; ++itemIndex) {
			const char* itemLabel = items[itemIndex] ? items[itemIndex] : "";
			const ImGuiID itemId = popupWindow->GetID(itemIndex);
			const ImVec2 itemPos = popupWindow->DC.CursorPos;
			const ImRect itemBb(
				itemPos,
				ImVec2(itemPos.x + boxBb.GetWidth(), itemPos.y + preset.itemHeight)
			);

			ImGui::ItemSize(itemBb);
			if (!ImGui::ItemAdd(itemBb, itemId))
				continue;

			bool itemHovered = false;
			bool itemHeld = false;
			const bool itemPressed = ImGui::ButtonBehavior(itemBb, itemId, &itemHovered, &itemHeld);
			const bool isSelected = itemIndex == *currentIndex;

			if (itemHovered || isSelected) {
				popupDrawList->AddRectFilled(
					itemBb.Min,
					itemBb.Max,
					ColorToU32(preset.itemHighlight),
					0.f
				);
			}

			const ImVec2 itemLabelSize = ImGui::CalcTextSize(itemLabel, nullptr, true);
			const float itemLabelY = itemBb.Min.y + (preset.itemHeight - itemLabelSize.y) * 0.5f;
			const ImVec4 itemColor = itemHovered ? preset.textHovered : preset.textColor;
			popupDrawList->AddText(
				ImVec2(itemBb.Min.x + textPadding, itemLabelY),
				ColorToU32(itemColor),
				itemLabel
			);

			if (itemPressed && itemIndex != *currentIndex) {
				*currentIndex = itemIndex;
				changed = true;
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::EndPopup();
	}

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);
	ImGui::PopID();

	return changed;
}

} // namespace Custom
