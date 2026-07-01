#pragma once

#include "../imgui.h"

namespace Custom {

ImVec4 ColorFromHex(unsigned int hex, float alpha = 1.f);
ImU32 ColorToU32(const ImVec4& color);

struct MainGuiPreset {
	ImVec4 background{};
	ImVec4 outline{};
	float outlineSize = 1.f;
	float rounding = 0.f;
	ImVec2 defaultSize{ 600, 430.f };
	ImVec2 padding{ 0.f, 0.f };
	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar;
};

struct TopBarPreset {
	ImVec4 background{};
	ImVec4 outlineColor{};
	float outlineSize = 1.f;
	float rounding = 3.f;
	ImVec4 separatorColor{};
	ImVec4 tabSelectedBackground{};
	ImVec4 tabActiveUnderline{};
	float tabActiveUnderlineHeight = 1.f;
	ImVec4 tabOutlineColor{};
	float tabOutlineSize = 1.f;
	float tabRounding = 3.f;
	ImVec4 titleColor{};
	ImVec4 tabTextActive{};
	ImVec4 tabTextInactive{};
	ImVec4 tabTextHovered{};
	float height = 34.f;
	float tabHeight = 28.f;
	ImVec2 titlePadding{ 12.f, 0.f };
	ImVec2 tabPadding{ 12.f, 0.f };
	float tabSpacing = 4.f;
};

struct FooterBarPreset {
	ImVec4 background{};
	ImVec4 outlineColor{};
	float outlineSize = 1.f;
	float rounding = 3.f;
	ImVec4 separatorColor{};
	ImVec4 labelColor{};
	ImVec4 valueColor{};
	float height = 24.f;
	ImVec2 padding{ 12.f, 0.f };
};

struct SectionPreset {
	ImVec4 background{};
	ImVec4 headerBackground{};
	ImVec4 borderColor{};
	float borderSize = 1.f;
	ImVec4 headerTextColor{};
	ImVec4 headerSeparatorColor{};
	float headerOutlineSize = 1.f;
	float headerHeight = 21.f;
	ImVec2 headerPadding{ 8.f, 2.f };
	ImVec2 padding{ 12.f, 6.f };
	float rounding = 0.f;
	float gap = 8.f;
	float splitRatio = 0.5f;
};

struct TogglePreset {
	ImVec4 boxBorder{};
	ImVec4 boxBorderHovered{};
	ImVec4 checkColor{};
	ImVec4 labelActive{};
	ImVec4 labelInactive{};
	float boxSize = 14.f;
	float spacing = 8.f;
	float rounding = 2.f;
	float fillInset = 2.f;
};

struct SliderPreset {
	ImVec4 trackColor{};
	ImVec4 fillColor{};
	ImVec4 grabColor{};
	ImVec4 labelColor{};
	ImVec4 valueColor{};
	float height = 6.f;
	float grabRadius = 5.f;
	float rounding = 0.f;
	float rowHeight = 28.f;
	float spacing = 8.f;
};

struct DropdownPreset {
	ImVec4 labelColor{};
	ImVec4 boxBackground{};
	ImVec4 boxBorder{};
	ImVec4 boxBorderHovered{};
	ImVec4 boxBorderOpen{};
	ImVec4 textColor{};
	ImVec4 textSelected{};
	ImVec4 textHovered{};
	ImVec4 popupBackground{};
	ImVec4 popupBorder{};
	ImVec4 itemHighlight{};
	ImVec4 arrowColor{};
	float rowHeight = 28.f;
	float boxHeight = 22.f;
	float spacing = 8.f;
	float rounding = 2.f;
	float popupRounding = 2.f;
	float itemHeight = 22.f;
	float arrowWidth = 18.f;
};

struct MultiComboPreset {
	ImVec4 labelColor{};
	ImVec4 boxBackground{};
	ImVec4 boxBorder{};
	ImVec4 boxBorderHovered{};
	ImVec4 boxBorderOpen{};
	ImVec4 textColor{};
	ImVec4 textSelected{};
	ImVec4 textHovered{};
	ImVec4 popupBackground{};
	ImVec4 popupBorder{};
	ImVec4 itemHighlight{};
	ImVec4 arrowColor{};
	ImVec4 checkBorder{};
	ImVec4 checkBorderHovered{};
	ImVec4 checkFill{};
	float rowHeight = 28.f;
	float boxHeight = 22.f;
	float spacing = 8.f;
	float rounding = 2.f;
	float popupRounding = 2.f;
	float itemHeight = 22.f;
	float arrowWidth = 18.f;
	float checkSize = 12.f;
	float checkSpacing = 8.f;
	float checkRounding = 2.f;
	float checkFillInset = 2.f;
};

struct ColorPickerPreset {
	ImVec4 labelColor{};
	ImVec4 previewBorder{};
	ImVec4 previewBorderHovered{};
	ImVec4 popupBackground{};
	ImVec4 popupBorder{};
	ImVec4 grabBorder{};
	float rowHeight = 28.f;
	float previewSize = 20.f;
	float spacing = 8.f;
	float rounding = 2.f;
	float svPickerSize = 140.f;
	float hueBarHeight = 12.f;
	float popupPadding = 8.f;
	float grabRadius = 5.f;
};

struct GuiPresets {
	MainGuiPreset mainGui{};
	TopBarPreset topBar{};
	FooterBarPreset footer{};
	SectionPreset section{};
	TogglePreset toggle{};
	SliderPreset slider{};
	DropdownPreset dropdown{};
	MultiComboPreset multiCombo{};
	ColorPickerPreset colorPicker{};
};

extern GuiPresets g_presets;

GuiPresets MakeDefaultPresets();

} // namespace Custom
