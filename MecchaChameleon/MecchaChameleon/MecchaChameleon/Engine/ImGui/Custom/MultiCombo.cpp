#include "MultiCombo.hpp"

#include "../imgui_internal.h"

#include <cfloat>
#include <cstdio>

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

	void BuildPreviewText(
		const char* const* items,
		unsigned int flags,
		int itemCount,
		char* out,
		int outSize
	) {
		if (!out || outSize <= 0)
			return;

		out[0] = '\0';

		int selectedCount = 0;
		for (int itemIndex = 0; itemIndex < itemCount; ++itemIndex) {
			if ((flags & MultiComboBit(itemIndex)) != 0)
				++selectedCount;
		}

		if (selectedCount == 0) {
			snprintf(out, outSize, "None");
			return;
		}

		if (selectedCount == itemCount) {
			snprintf(out, outSize, "All");
			return;
		}

		bool first = true;
		int written = 0;

		for (int itemIndex = 0; itemIndex < itemCount; ++itemIndex) {
			if ((flags & MultiComboBit(itemIndex)) == 0)
				continue;

			const char* itemLabel = items[itemIndex] ? items[itemIndex] : "";
			const int added = snprintf(
				out + written,
				outSize - written,
				"%s%s",
				first ? "" : ", ",
				itemLabel
			);

			if (added < 0 || written + added >= outSize) {
				if (written + 3 < outSize)
					snprintf(out + written, outSize - written, "...");
				break;
			}

			written += added;
			first = false;
		}
	}

	void DrawCheckBox(
		ImDrawList* drawList,
		const ImRect& boxBb,
		bool checked,
		bool hovered,
		const MultiComboPreset& preset
	) {
		const ImVec4 borderColor = hovered ? preset.checkBorderHovered : preset.checkBorder;
		drawList->AddRect(
			boxBb.Min,
			boxBb.Max,
			ColorToU32(borderColor),
			preset.checkRounding,
			0,
			1.f
		);

		if (!checked)
			return;

		const float inset = preset.checkFillInset;
		const float innerSize = preset.checkSize - inset * 2.f;
		const float fillRounding = ImMin(preset.checkRounding, innerSize * 0.5f - 0.01f);
		drawList->AddRectFilled(
			ImVec2(boxBb.Min.x + inset, boxBb.Min.y + inset),
			ImVec2(boxBb.Max.x - inset, boxBb.Max.y - inset),
			ColorToU32(preset.checkFill),
			fillRounding
		);
	}
}

bool MultiCombo(
	const char* label,
	unsigned int* flags,
	const char* const* items,
	int itemCount,
	const MultiComboPreset& preset
) {
	if (!flags || !items || itemCount <= 0 || itemCount > static_cast<int>(kMultiComboMaxItems))
		return false;

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	const ImGuiID id = window->GetID(label);
	const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);

	char previewText[128];
	BuildPreviewText(items, *flags, itemCount, previewText, IM_ARRAYSIZE(previewText));
	const ImVec2 previewSize = ImGui::CalcTextSize(previewText, nullptr, true);

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

	bool changed = false;

	ImGui::PushID(id);
	if (pressed)
		ImGui::OpenPopup("##multicombo_popup");

	const bool popupOpen = ImGui::IsPopupOpen("##multicombo_popup", ImGuiPopupFlags_None);
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
	const float textY = boxBb.Min.y + (preset.boxHeight - previewSize.y) * 0.5f;
	const float textMaxX = boxBb.Max.x - preset.arrowWidth;
	drawList->PushClipRect(
		ImVec2(boxBb.Min.x + textPadding, boxBb.Min.y),
		ImVec2(textMaxX, boxBb.Max.y),
		true
	);
	drawList->AddText(
		ImVec2(boxBb.Min.x + textPadding, textY),
		ColorToU32(preset.textSelected),
		previewText
	);
	drawList->PopClipRect();

	const ImVec2 arrowCenter(
		boxBb.Max.x - preset.arrowWidth * 0.5f,
		boxBb.Min.y + preset.boxHeight * 0.5f
	);
	DrawArrow(drawList, arrowCenter, 8.f, ColorToU32(preset.arrowColor));

	ImGui::SetNextWindowPos(ImVec2(boxBb.Min.x, boxBb.Max.y + 2.f), ImGuiCond_Appearing);
	ImGui::SetNextWindowSizeConstraints(
		ImVec2(boxBb.GetWidth(), 0.f),
		ImVec2(boxBb.GetWidth(), FLT_MAX)
	);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, preset.popupRounding);
	ImGui::PushStyleColor(ImGuiCol_PopupBg, preset.popupBackground);
	ImGui::PushStyleColor(ImGuiCol_Border, preset.popupBorder);

	if (ImGui::BeginPopup("##multicombo_popup", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
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
			const unsigned int itemBit = MultiComboBit(itemIndex);
			const bool isChecked = (*flags & itemBit) != 0;

			if (itemHovered || isChecked) {
				popupDrawList->AddRectFilled(
					itemBb.Min,
					itemBb.Max,
					ColorToU32(preset.itemHighlight),
					0.f
				);
			}

			const float checkOffsetY = (preset.itemHeight - preset.checkSize) * 0.5f;
			const ImRect checkBb(
				ImVec2(itemBb.Min.x + textPadding, itemBb.Min.y + checkOffsetY),
				ImVec2(itemBb.Min.x + textPadding + preset.checkSize, itemBb.Min.y + checkOffsetY + preset.checkSize)
			);
			DrawCheckBox(popupDrawList, checkBb, isChecked, itemHovered, preset);

			const ImVec2 itemLabelSize = ImGui::CalcTextSize(itemLabel, nullptr, true);
			const float itemLabelY = itemBb.Min.y + (preset.itemHeight - itemLabelSize.y) * 0.5f;
			const ImVec4 itemColor = itemHovered ? preset.textHovered : preset.textColor;
			popupDrawList->AddText(
				ImVec2(checkBb.Max.x + preset.checkSpacing, itemLabelY),
				ColorToU32(itemColor),
				itemLabel
			);

			if (itemPressed) {
				*flags ^= itemBit;
				changed = true;
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
