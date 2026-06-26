#include "FooterBar.hpp"

namespace Custom {

void FooterBar(const char* version, const FooterBarPreset& preset) {
	const float width = ImGui::GetWindowWidth();
	const float footerY = ImGui::GetWindowHeight() - preset.height;
	ImGui::SetCursorPos(ImVec2(0.f, footerY));

	const ImVec2 barMin = ImGui::GetCursorScreenPos();
	const ImVec2 barMax(barMin.x + width, barMin.y + preset.height);
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	constexpr ImDrawFlags kFooterCorners = ImDrawFlags_RoundCornersBottom;

	drawList->AddRectFilled(barMin, barMax, ColorToU32(preset.background), preset.rounding, kFooterCorners);
	if (preset.outlineSize > 0.f) {
		drawList->AddRect(
			barMin,
			barMax,
			ColorToU32(preset.outlineColor),
			preset.rounding,
			kFooterCorners,
			preset.outlineSize
		);
	}

	drawList->AddLine(
		barMin,
		ImVec2(barMax.x, barMin.y),
		ColorToU32(preset.separatorColor),
		1.f
	);

	const char* label = "version: ";
	const ImVec2 labelSize = ImGui::CalcTextSize(label);
	const float textY = barMin.y + (preset.height - ImGui::GetTextLineHeight()) * 0.5f;
	const float textX = barMin.x + preset.padding.x;

	drawList->AddText(ImVec2(textX, textY), ColorToU32(preset.labelColor), label);
	drawList->AddText(
		ImVec2(textX + labelSize.x, textY),
		ColorToU32(preset.valueColor),
		version
	);

	ImGui::Dummy(ImVec2(width, preset.height));
}

} // namespace Custom
