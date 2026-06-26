#include "Presets.hpp"

namespace Custom {

ImVec4 ColorFromHex(unsigned int hex, float alpha) {
	const float r = static_cast<float>((hex >> 16) & 0xFF) / 255.f;
	const float g = static_cast<float>((hex >> 8) & 0xFF) / 255.f;
	const float b = static_cast<float>(hex & 0xFF) / 255.f;
	return ImVec4(r, g, b, alpha);
}

ImU32 ColorToU32(const ImVec4& color) {
	return ImGui::ColorConvertFloat4ToU32(color);
}

GuiPresets g_presets = MakeDefaultPresets();

GuiPresets MakeDefaultPresets() {
	GuiPresets presets{};

	presets.mainGui.background = ColorFromHex(0x0A0A0A);
	presets.mainGui.outline = ColorFromHex(0x292929);
	presets.mainGui.outlineSize = 1.f;
	presets.mainGui.rounding = 3.f;
	presets.mainGui.defaultSize = ImVec2(520.f, 400.f);
	presets.mainGui.padding = ImVec2(8.f, 0.f);

	presets.topBar.background = ColorFromHex(0x101010);
	presets.topBar.outlineColor = ColorFromHex(0x292929);
	presets.topBar.outlineSize = 1.f;
	presets.topBar.rounding = 3.f;
	presets.topBar.separatorColor = ColorFromHex(0x292929);
	presets.topBar.tabSelectedBackground = ColorFromHex(0x212122);
	presets.topBar.tabActiveUnderline = ColorFromHex(0xC46B84);
	presets.topBar.tabActiveUnderlineHeight = 1.f;
	presets.topBar.tabOutlineColor = ColorFromHex(0x292929);
	presets.topBar.tabOutlineSize = 1.f;
	presets.topBar.tabRounding = 3.f;
	presets.topBar.titleColor = ColorFromHex(0xE8E8E8);
	presets.topBar.tabTextActive = ColorFromHex(0xE8E8E8);
	presets.topBar.tabTextInactive = ColorFromHex(0x6E6E6E);
	presets.topBar.tabTextHovered = ColorFromHex(0xA0A0A0);
	presets.topBar.height = 34.f;
	presets.topBar.tabHeight = 28.f;
	presets.topBar.titlePadding = ImVec2(12.f, 0.f);
	presets.topBar.tabPadding = ImVec2(12.f, 0.f);
	presets.topBar.tabSpacing = 4.f;

	presets.footer.background = ColorFromHex(0x101010);
	presets.footer.outlineColor = ColorFromHex(0x292929);
	presets.footer.outlineSize = 1.f;
	presets.footer.rounding = 3.f;
	presets.footer.separatorColor = ColorFromHex(0x292929);
	presets.footer.labelColor = ColorFromHex(0xE8E8E8);
	presets.footer.valueColor = ColorFromHex(0xC46B84);
	presets.footer.height = 24.f;
	presets.footer.padding = ImVec2(12.f, 0.f);

	presets.section.background = ColorFromHex(0x141414);
	presets.section.headerBackground = ColorFromHex(0x171717);
	presets.section.borderColor = ColorFromHex(0x292929);
	presets.section.borderSize = 1.f;
	presets.section.headerTextColor = ColorFromHex(0xE8E8E8);
	presets.section.headerSeparatorColor = ColorFromHex(0x292929);
	presets.section.headerOutlineSize = 1.f;
	presets.section.headerHeight = 21.f;
	presets.section.headerPadding = ImVec2(8.f, 2.f);
	presets.section.padding = ImVec2(8.f, 6.f);
	presets.section.rounding = 0.f;
	presets.section.gap = 8.f;
	presets.section.splitRatio = 0.5f;

	presets.toggle.boxBorder = ColorFromHex(0x292929);
	presets.toggle.boxBorderHovered = ColorFromHex(0xC46B84);
	presets.toggle.checkColor = ColorFromHex(0xC46B84);
	presets.toggle.labelActive = ColorFromHex(0xE8E8E8);
	presets.toggle.labelInactive = ColorFromHex(0x6E6E6E);
	presets.toggle.boxSize = 14.f;
	presets.toggle.spacing = 8.f;
	presets.toggle.rounding = 2.f;
	presets.toggle.fillInset = 2.f;

	presets.slider.trackColor = ColorFromHex(0x101010);
	presets.slider.fillColor = ColorFromHex(0xC46B84, 0.55f);
	presets.slider.grabColor = ColorFromHex(0xE8E8E8);
	presets.slider.labelColor = ColorFromHex(0xE8E8E8);
	presets.slider.valueColor = ColorFromHex(0x6E6E6E);
	presets.slider.height = 6.f;
	presets.slider.grabRadius = 5.f;
	presets.slider.rounding = 0.f;
	presets.slider.rowHeight = 28.f;
	presets.slider.spacing = 8.f;

	return presets;
}

} // namespace Custom
