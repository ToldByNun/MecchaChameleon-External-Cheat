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
#include "../../Engine/ImGui/Custom/Dropdown.hpp"
#include "../../Engine/ImGui/Custom/ColorPicker.hpp"
#include "../../Engine/ImGui/Custom/MultiCombo.hpp"

struct MenuGenerals {
	enum MenuCategory {
		COMBAT,
		VISUALS,
		EXPLOITS,
		TAB_COUNT
	};

	static inline int activeCategory = 0;

	static inline const char* categories[] = {
		"Combat", "Visuals", "Exploits"
	};

	static inline const char* teamOptions[] = {
		"Enemy", "Teammate"
	};

	static inline const char* hitboxOptions[] = {
		"Head", "Chest", "Stomach", "Hip", "Left Arm", "Right Arm", "Left Knee", "Right Knee"
	};

	static void renderColorWidget(const char* name, ImVec4& defaultColor, ImVec4& enemyColor) {
		globals.settings.esp.selectedTeam == 0 ?
			Custom::ColorPicker(name, &defaultColor) :
			Custom::ColorPicker(name, &enemyColor);
	}
};

void Menu::handleInput() {
	if (GetAsyncKeyState(VK_INSERT) & 1)
		globals.settings.menuOpen = !globals.settings.menuOpen;
}

struct ESPTab {
	static void renderEspSettings() {
		Custom::Toggle("Show FoV", &globals.settings.esp.fovCircle);
		Custom::Toggle("Minimap", &globals.settings.esp.minimap);
		Custom::Toggle("Box ESP", &globals.settings.esp.box);
		Custom::Toggle("Corner ESP", &globals.settings.esp.corners);
		MenuGenerals::renderColorWidget("Box Color", globals.settings.esp.defaultBoxColor, globals.settings.esp.enemyBoxColor);
		Custom::Toggle("Dynamic Boxes", &globals.settings.esp.dynamicBoxes);
		Custom::Toggle("Skeleton ESP", &globals.settings.esp.skeleton);
		MenuGenerals::renderColorWidget("Skeleton Color", globals.settings.esp.defaultSkeletonColor, globals.settings.esp.enemySkeletonColor);
		Custom::Toggle("Chinese Hat", &globals.settings.esp.chineseHat);
		Custom::Toggle("Name", &globals.settings.esp.name);
		Custom::Toggle("Distance", &globals.settings.esp.distance);
		Custom::Toggle("Snaplines", &globals.settings.esp.snaplines);
		MenuGenerals::renderColorWidget("Snaplines Color", globals.settings.esp.defaultSnaplineColor, globals.settings.esp.enemySnaplineColor);
	}

	static void renderEspOptions() {
		Custom::Dropdown("Team", &globals.settings.esp.selectedTeam, MenuGenerals::teamOptions, IM_ARRAYSIZE(MenuGenerals::teamOptions));
		Custom::Toggle("Hide Teammates", &globals.settings.esp.onlyEnemies);
		Custom::Toggle("Change Enemy Color", &globals.settings.esp.isTeammateColorEnabled);
		Custom::Toggle("Hide Enemies MM", &globals.settings.esp.hideEnemiesMM);
		Custom::Toggle("Hide Teammates MM", &globals.settings.esp.hideTeammatesMM);
		MenuGenerals::renderColorWidget("Minimap Color", globals.settings.esp.defaultMMColor, globals.settings.esp.enemyMMColor);
	}
};

struct AimbotTab {
	static void renderAimbotSettings() {
		Custom::Toggle("Enabled", &globals.settings.aimbot.enabled);
		Custom::Toggle("FOV limit", &globals.settings.aimbot.fovLimit);
		Custom::Toggle("Smoothing", &globals.settings.aimbot.smoothing);
	}

	static void renderAimbotOptions() {
		Custom::MultiCombo("Hitboxes", &globals.settings.aimbot.hitboxMask, MenuGenerals::hitboxOptions, kHitboxFlagCount);
		Custom::SliderFloat("FOV", &globals.settings.aimbot.fov, 1.f, 180.f, "%.0f");
		Custom::SliderFloat("Smooth", &globals.settings.aimbot.smooth, 1.f, 20.f, "%.1f");
	}
};

struct ExploitTab {
	static void renderExploitsSettings() {
		///////////
	}

	static void renderExploitsOptions() {
		///////////
	}
};

static const char* getLeftSectionTitle(int category) {
	switch (category) {
	case MenuGenerals::COMBAT:   return "Aimbot";
	case MenuGenerals::VISUALS:  return "ESP";
	case MenuGenerals::EXPLOITS: return "Exploits";
	default: return "";
	}
}

static void renderLeftSection(int category) {
	switch (category) {
	case MenuGenerals::COMBAT:   AimbotTab::renderAimbotSettings(); break;
	case MenuGenerals::VISUALS:  ESPTab::renderEspSettings(); break;
	case MenuGenerals::EXPLOITS: ExploitTab::renderExploitsSettings(); break;
	}
}

static void renderRightSection(int category) {
	switch (category) {
	case MenuGenerals::COMBAT:   AimbotTab::renderAimbotOptions(); break;
	case MenuGenerals::VISUALS:  ESPTab::renderEspOptions(); break;
	case MenuGenerals::EXPLOITS: ExploitTab::renderExploitsOptions(); break;
	}
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

	Custom::TopBar(
		"MecchaChameleon",
		MenuGenerals::categories,
		MenuGenerals::TAB_COUNT,
		&MenuGenerals::activeCategory,
		topPreset
	);

	const float contentHeight = Custom::GetContentHeight(topPreset.height, footerPreset.height);
	const float contentWidth = ImGui::GetWindowWidth() - mainPreset.padding.x * 2.f;
	ImGui::SetCursorPos(ImVec2(mainPreset.padding.x, topPreset.height));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::BeginChild("Content", ImVec2(contentWidth, contentHeight), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);

	const float availHeight = ImGui::GetContentRegionAvail().y;
	const float sectionMargin = 9.f;
	const float sectionHeight = availHeight - sectionMargin * 2.f;
	Custom::BeginSectionDualLayout(sectionHeight);

	if (Custom::BeginSectionDualLeft(getLeftSectionTitle(MenuGenerals::activeCategory))) {
		renderLeftSection(MenuGenerals::activeCategory);
	}
	Custom::EndSectionDualLeft();

	if (Custom::BeginSectionDualRight("Options")) {
		renderRightSection(MenuGenerals::activeCategory);
	}
	Custom::EndSectionDualRight();

	Custom::EndSectionDualLayout();

	ImGui::EndChild();
	ImGui::PopStyleColor();

	Custom::FooterBar("0.0.3", footerPreset);
	Custom::EndMainGui();
}
