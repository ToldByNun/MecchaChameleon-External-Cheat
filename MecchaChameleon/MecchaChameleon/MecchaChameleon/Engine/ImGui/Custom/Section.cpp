#include "Section.hpp"

namespace Custom {

namespace {
	struct SectionState {
		ImVec2 origin{};
		ImVec2 size{};
		bool active = false;
	};

	struct DualLayoutState {
		bool active = false;
		float height = 0.f;
		float leftWidth = 0.f;
		float rightWidth = 0.f;
		float gap = 0.f;
		ImVec2 rowStart{};
		SectionPreset preset{};
	};

	SectionState g_section{};
	DualLayoutState g_dual{};
}

bool BeginSection(const char* label, ImVec2 size, const SectionPreset& preset) {
	const ImVec2 avail = ImGui::GetContentRegionAvail();
	if (size.x <= 0.f)
		size.x = avail.x;
	if (size.y <= 0.f)
		size.y = avail.y;

	const ImVec2 origin = ImGui::GetCursorScreenPos();
	const ImVec2 sectionMax(origin.x + size.x, origin.y + size.y);
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	drawList->AddRectFilled(origin, sectionMax, ColorToU32(preset.background));
	drawList->AddRect(
		origin,
		sectionMax,
		ColorToU32(preset.borderColor),
		preset.rounding,
		0,
		preset.borderSize
	);

	const ImVec2 headerMax(origin.x + size.x, origin.y + preset.headerHeight);
	drawList->AddRectFilled(origin, headerMax, ColorToU32(preset.headerBackground), preset.rounding, ImDrawFlags_RoundCornersTop);
	if (preset.headerOutlineSize > 0.f) {
		drawList->AddRect(
			origin,
			headerMax,
			ColorToU32(preset.borderColor),
			preset.rounding,
			ImDrawFlags_RoundCornersTop,
			preset.headerOutlineSize
		);
	}
	drawList->AddLine(
		ImVec2(origin.x, headerMax.y),
		ImVec2(sectionMax.x, headerMax.y),
		ColorToU32(preset.headerSeparatorColor),
		1.f
	);

	const float headerTextHeight = ImGui::GetTextLineHeight();
	const float textY = origin.y + (preset.headerHeight - headerTextHeight) * 0.5f;
	drawList->AddText(
		ImVec2(origin.x + preset.headerPadding.x, textY),
		ColorToU32(preset.headerTextColor),
		label
	);

	ImGui::SetCursorScreenPos(ImVec2(origin.x + preset.padding.x, headerMax.y + preset.padding.y));
	const float innerWidth = size.x - preset.padding.x * 2.f;
	const float innerHeight = size.y - preset.headerHeight - preset.padding.y * 2.f;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, 0.f));
	const bool opened = ImGui::BeginChild(
		label,
		ImVec2(innerWidth, innerHeight),
		ImGuiChildFlags_None,
		ImGuiWindowFlags_NoScrollbar
	);

	g_section = { origin, size, opened };
	return opened;
}

void EndSection() {
	const ImVec2 origin = g_section.origin;
	const ImVec2 size = g_section.size;

	if (g_section.active) {
		ImGui::EndChild();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
		g_section.active = false;
	}

	ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + size.y));
	ImGui::Dummy(ImVec2(size.x, 0.f));
}

void BeginSectionDualLayout(float height, const SectionPreset& preset) {
	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const float totalWidth = avail.x;
	const float gap = preset.gap;
	const float leftWidth = (totalWidth - gap) * preset.splitRatio;
	const float rightWidth = totalWidth - gap - leftWidth;

	float sectionHeight = height;
	if (sectionHeight <= 0.f)
		sectionHeight = avail.y;

	const float offsetY = (avail.y - sectionHeight) * 0.5f;
	if (offsetY > 0.f)
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);

	g_dual.active = true;
	g_dual.height = height;
	g_dual.leftWidth = leftWidth;
	g_dual.rightWidth = rightWidth;
	g_dual.gap = gap;
	g_dual.rowStart = ImGui::GetCursorScreenPos();
	g_dual.preset = preset;

	ImGui::BeginGroup();
}

bool BeginSectionDualLeft(const char* label, const SectionPreset& preset) {
	if (!g_dual.active)
		return false;

	ImGui::SetCursorScreenPos(g_dual.rowStart);
	return BeginSection(label, ImVec2(g_dual.leftWidth, g_dual.height), preset);
}

void EndSectionDualLeft() {
	EndSection();
}

bool BeginSectionDualRight(const char* label, const SectionPreset& preset) {
	if (!g_dual.active)
		return false;

	const ImVec2 rightStart(
		g_dual.rowStart.x + g_dual.leftWidth + g_dual.gap,
		g_dual.rowStart.y
	);
	ImGui::SetCursorScreenPos(rightStart);
	return BeginSection(label, ImVec2(g_dual.rightWidth, g_dual.height), preset);
}

void EndSectionDualRight() {
	EndSection();
}

void EndSectionDualLayout() {
	if (!g_dual.active)
		return;

	ImGui::EndGroup();
	ImGui::Dummy(ImVec2(0.f, g_dual.height));
	g_dual.active = false;
}

} // namespace Custom
