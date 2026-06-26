#include "TopBar.hpp"

namespace Custom {

namespace {
	ImVec2 MeasureTextSize(const char* text) {
		return ImGui::CalcTextSize(text);
	}

	constexpr ImDrawFlags kTopBarCorners = ImDrawFlags_RoundCornersTop;
	constexpr ImDrawFlags kTabCorners = ImDrawFlags_RoundCornersTop;
}

int TopBar(
	const char* title,
	const char* const* categories,
	int count,
	int* activeIndex,
	const TopBarPreset& preset
) {
	if (!activeIndex || count <= 0)
		return activeIndex ? *activeIndex : 0;

	if (*activeIndex < 0)
		*activeIndex = 0;
	if (*activeIndex >= count)
		*activeIndex = count - 1;

	ImGui::SetCursorPos(ImVec2(0.f, 0.f));

	const ImVec2 winPos = ImGui::GetWindowPos();
	const float width = ImGui::GetWindowWidth();
	const ImVec2 barMin = winPos;
	const ImVec2 barMax(winPos.x + width, winPos.y + preset.height);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(barMin, barMax, ColorToU32(preset.background), preset.rounding, kTopBarCorners);
	if (preset.outlineSize > 0.f) {
		drawList->AddRect(
			barMin,
			barMax,
			ColorToU32(preset.outlineColor),
			preset.rounding,
			kTopBarCorners,
			preset.outlineSize
		);
	}

	const float textY = barMin.y + (preset.height - ImGui::GetTextLineHeight()) * 0.5f;
	const ImVec2 titleSize = MeasureTextSize(title);
	const float titleX = barMin.x + preset.titlePadding.x;

	drawList->AddText(ImVec2(titleX, textY), ColorToU32(preset.titleColor), title);

	float tabX = titleX + titleSize.x + preset.tabSpacing * 2.f;
	const float tabTop = barMax.y - preset.tabHeight;

	for (int i = 0; i < count; ++i) {
		const char* label = categories[i];
		const ImVec2 labelSize = MeasureTextSize(label);
		const float tabWidth = labelSize.x + preset.tabPadding.x * 2.f;
		const ImVec2 tabMin(tabX, tabTop);
		const ImVec2 tabMax(tabX + tabWidth, barMax.y);

		const bool isActive = i == *activeIndex;
		const bool hovered = ImGui::IsMouseHoveringRect(tabMin, tabMax);
		const float tabTextY = tabMin.y + (preset.tabHeight - ImGui::GetTextLineHeight()) * 0.5f;

		if (isActive) {
			drawList->AddRectFilled(
				tabMin,
				tabMax,
				ColorToU32(preset.tabSelectedBackground),
				preset.tabRounding,
				kTabCorners
			);
			if (preset.tabActiveUnderlineHeight > 0.f) {
				const float accentInset = preset.tabRounding;
				const float lineY = tabMin.y + preset.tabActiveUnderlineHeight * 0.5f;
				drawList->AddLine(
					ImVec2(tabMin.x + accentInset, lineY),
					ImVec2(tabMax.x - accentInset, lineY),
					ColorToU32(preset.tabActiveUnderline),
					preset.tabActiveUnderlineHeight
				);
			}
		}

		if (preset.tabOutlineSize > 0.f) {
			drawList->AddRect(
				tabMin,
				tabMax,
				ColorToU32(preset.tabOutlineColor),
				preset.tabRounding,
				kTabCorners,
				preset.tabOutlineSize
			);
		}

		ImVec4 textColor = preset.tabTextInactive;
		if (isActive)
			textColor = preset.tabTextActive;
		else if (hovered)
			textColor = preset.tabTextHovered;

		const float labelX = tabMin.x + preset.tabPadding.x;
		drawList->AddText(ImVec2(labelX, tabTextY), ColorToU32(textColor), label);

		ImGui::SetCursorScreenPos(tabMin);
		ImGui::PushID(i);
		if (ImGui::InvisibleButton("##tab", ImVec2(tabWidth, preset.tabHeight)) && !isActive)
			*activeIndex = i;
		ImGui::PopID();

		tabX += tabWidth + preset.tabSpacing;
	}

	drawList->AddLine(
		ImVec2(barMin.x, barMax.y),
		ImVec2(barMax.x, barMax.y),
		ColorToU32(preset.separatorColor),
		1.f
	);

	ImGui::SetCursorPos(ImVec2(0.f, preset.height));
	ImGui::Dummy(ImVec2(width, 0.f));

	return *activeIndex;
}

} // namespace Custom
