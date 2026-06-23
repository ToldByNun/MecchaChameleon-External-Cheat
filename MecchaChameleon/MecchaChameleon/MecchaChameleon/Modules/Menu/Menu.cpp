#include "Menu.hpp"
#include "../../Engine/ImGui/imgui.h"

namespace settings {
	bool menuOpen = true;
	EspSettings esp{};
	AimbotSettings aimbot{};
}

void Menu::handleInput() {
	if (GetAsyncKeyState(VK_INSERT) & 1)
		settings::menuOpen = !settings::menuOpen;
}

void Menu::render() {
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

	if (!settings::menuOpen) {
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
	ImGui::Begin("MecchaChameleon", &settings::menuOpen, ImGuiWindowFlags_NoCollapse);

	if (ImGui::BeginTabBar("MainTabs")) {
		if (ImGui::BeginTabItem("ESP")) {
			ImGui::Checkbox("Box ESP", &settings::esp.box);
			ImGui::Checkbox("Skeleton ESP", &settings::esp.skeleton);
			ImGui::Checkbox("Name / Distance", &settings::esp.nameDistance);
			ImGui::Checkbox("Snaplines", &settings::esp.snaplines);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Aimbot")) {
			ImGui::Checkbox("Enabled", &settings::aimbot.enabled);
			ImGui::Checkbox("FOV limit", &settings::aimbot.fovLimit);
			if (settings::aimbot.fovLimit)
				ImGui::SliderFloat("FOV", &settings::aimbot.fov, 1.f, 180.f, "%.0f");
			ImGui::Checkbox("Smoothing", &settings::aimbot.smoothing);
			if (settings::aimbot.smoothing)
				ImGui::SliderFloat("Smooth", &settings::aimbot.smooth, 1.f, 20.f, "%.1f");
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Misc")) {
			ImGui::Text("INSERT — toggle menu");
			ImGui::Text("Research build — features are placeholders.");
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

