#include "Menu.hpp"

#include "../../Manager/Globals/Globals.hpp"
#include "../../Engine/ImGui/imgui.h"
#include "../../Engine/ImGui/Custom/MainGui.hpp"
#include "../../Engine/ImGui/Custom/TopBar.hpp"
#include "../../Engine/ImGui/Custom/FooterBar.hpp"
#include "../../Engine/ImGui/Custom/Section.hpp"
#include "../../Engine/ImGui/Custom/Toggle.hpp"
#include "../../Engine/ImGui/Custom/Slider.hpp"
#include "../../Engine/ImGui/Custom/Presets.hpp"

static int activeCategory = 0;

static const char* categories[] = { "Combat", "Visuals" };

void Menu::handleInput() {
	if (GetAsyncKeyState(VK_INSERT) & 1)
		globals.settings.menuOpen = !globals.settings.menuOpen;
}

static void renderEspSettings() {
	Custom::Toggle("Show FoV", &globals.settings.esp.fovCircle);
	Custom::Toggle("Box ESP", &globals.settings.esp.box);
	Custom::Toggle("Corner ESP", &globals.settings.esp.corners);
	Custom::Toggle("Chinese Hat", &globals.settings.esp.chineseHat);
	Custom::Toggle("Name", &globals.settings.esp.name);
	Custom::Toggle("Distance", &globals.settings.esp.distance);
	Custom::Toggle("Snaplines", &globals.settings.esp.snaplines);
}

static void renderEspOptions() {
	if (globals.settings.esp.fovCircle)
		Custom::SliderFloat("FoV", &globals.settings.aimbot.fov, 1.f, 180.f, "%.0f");
	Custom::Toggle("Hide Teammates", &globals.settings.esp.onlyEnemies);
	Custom::Toggle("Change Enemy Color", &globals.settings.esp.isTeammateColorEnabled);
}

static void renderAimbotSettings() {
	Custom::Toggle("Enabled", &globals.settings.aimbot.enabled);
	Custom::Toggle("FOV limit", &globals.settings.aimbot.fovLimit);
	Custom::Toggle("Smoothing", &globals.settings.aimbot.smoothing);
}

static void renderAimbotOptions() {
	if (globals.settings.aimbot.fovLimit)
		Custom::SliderFloat("FOV", &globals.settings.aimbot.fov, 1.f, 180.f, "%.0f");
	if (globals.settings.aimbot.smoothing)
		Custom::SliderFloat("Smooth", &globals.settings.aimbot.smooth, 1.f, 20.f, "%.1f");
}

void Menu::render() {
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

	if (!globals.settings.menuOpen) {
		ImGui::GetBackgroundDrawList()->AddText(
			ImVec2(10.f, 10.f),
			IM_COL32(255, 255, 255, 230),
			"INSERT - Menu"
		);
		return;
	}

	const Custom::MainGuiPreset& mainPreset = Custom::g_presets.mainGui;
	const Custom::TopBarPreset& topPreset = Custom::g_presets.topBar;
	const Custom::FooterBarPreset& footerPreset = Custom::g_presets.footer;

	ImGui::SetNextWindowPos(
		ImVec2(
			displaySize.x * 0.5f - mainPreset.defaultSize.x * 0.5f,
			displaySize.y * 0.5f - mainPreset.defaultSize.y * 0.5f
		),
		ImGuiCond_FirstUseEver
	);
	ImGui::SetNextWindowSize(mainPreset.defaultSize, ImGuiCond_FirstUseEver);

	if (!Custom::BeginMainGui("MecchaChameleon", &globals.settings.menuOpen, mainPreset))
		return;

	Custom::TopBar("MecchaChameleon", categories, 2, &activeCategory, topPreset);

	const float contentHeight = Custom::GetContentHeight(topPreset.height, footerPreset.height);
	const float contentWidth = ImGui::GetWindowWidth() - mainPreset.padding.x * 2.f;
	ImGui::SetCursorPos(ImVec2(mainPreset.padding.x, topPreset.height));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::BeginChild("Content", ImVec2(contentWidth, contentHeight), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);

	const float availHeight = ImGui::GetContentRegionAvail().y;
	const float sectionMargin = 9.f;
	const float sectionHeight = availHeight - sectionMargin * 2.f;
	Custom::BeginSectionDualLayout(sectionHeight);

	if (Custom::BeginSectionDualLeft(activeCategory == 0 ? "Aimbot" : "ESP")) {
		if (activeCategory == 0)
			renderAimbotSettings();
		else
			renderEspSettings();
	}
	Custom::EndSectionDualLeft();

	if (Custom::BeginSectionDualRight("Options")) {
		if (activeCategory == 0)
			renderAimbotOptions();
		else
			renderEspOptions();
	}
	Custom::EndSectionDualRight();

	Custom::EndSectionDualLayout();

	ImGui::EndChild();
	ImGui::PopStyleColor();

	Custom::FooterBar("0.0.2", footerPreset);
	Custom::EndMainGui();
}
