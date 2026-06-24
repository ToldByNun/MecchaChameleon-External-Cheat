#include "Menu.hpp"

#include "../../Manager/Globals/Globals.hpp"
#include "../../Engine/ImGui/imgui.h"

void Menu::handleInput() {
	if (GetAsyncKeyState(VK_INSERT) & 1)
		globals.settings.menuOpen = !globals.settings.menuOpen;
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

	ImGui::SetNextWindowPos(
		ImVec2(displaySize.x * 0.5f - 210.f, displaySize.y * 0.5f - 180.f),
		ImGuiCond_FirstUseEver
	);
	ImGui::SetNextWindowSize(ImVec2(420.f, 360.f), ImGuiCond_FirstUseEver);
	ImGui::Begin("MecchaChameleon", &globals.settings.menuOpen, ImGuiWindowFlags_NoCollapse);

	if (ImGui::BeginTabBar("MainTabs")) {
		if (ImGui::BeginTabItem("ESP")) {
			ImGui::Checkbox("Box ESP", &globals.settings.esp.box);
			ImGui::Checkbox("Skeleton ESP", &globals.settings.esp.skeleton);
			ImGui::Checkbox("Name", &globals.settings.esp.name);
			ImGui::Checkbox("Distance", &globals.settings.esp.distance);
			ImGui::Checkbox("Snaplines", &globals.settings.esp.snaplines);
			ImGui::Checkbox("Hide Teammates", &globals.settings.esp.onlyEnemies);
			ImGui::Checkbox("Change Enemy Color", &globals.settings.esp.isTeammateColorEnabled);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Aimbot")) {
			ImGui::Checkbox("Enabled", &globals.settings.aimbot.enabled);
			ImGui::Checkbox("FOV limit", &globals.settings.aimbot.fovLimit);
			if (globals.settings.aimbot.fovLimit)
				ImGui::SliderFloat("FOV", &globals.settings.aimbot.fov, 1.f, 180.f, "%.0f");
			ImGui::Checkbox("Smoothing", &globals.settings.aimbot.smoothing);
			if (globals.settings.aimbot.smoothing)
				ImGui::SliderFloat("Smooth", &globals.settings.aimbot.smooth, 1.f, 20.f, "%.1f");
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Misc")) {
			ImGui::Text("INSERT - toggle menu");
			ImGui::Text("Research build - features are placeholders.");
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}
