#include "Menu.hpp"
#include "../../Engine/ImGui/imgui.h"

namespace {
	bool menuOpen = true;

	struct EspSettings {
		bool box = false;
		bool skeleton = false;
		bool nameDistance = false;
		bool snaplines = false;
	} esp;

	struct AimbotSettings {
		bool enabled = false;
		bool fovLimit = false;
		bool smoothing = false;
		float fov = 90.f;
		float smooth = 5.f;
	} aimbot;
}

namespace Menu {

bool isOpen() {
	return menuOpen;
}

void handleInput() {
	if (GetAsyncKeyState(VK_INSERT) & 1)
		menuOpen = !menuOpen;
}

void render() {
	if (!menuOpen)
		return;

	ImGui::SetNextWindowSize(ImVec2(420.f, 360.f), ImGuiCond_FirstUseEver);
	ImGui::Begin("MecchaChameleon", &menuOpen, ImGuiWindowFlags_NoCollapse);

	if (ImGui::BeginTabBar("MainTabs")) {
		if (ImGui::BeginTabItem("ESP")) {
			ImGui::Checkbox("Box ESP", &esp.box);
			ImGui::Checkbox("Skeleton ESP", &esp.skeleton);
			ImGui::Checkbox("Name / Distance", &esp.nameDistance);
			ImGui::Checkbox("Snaplines", &esp.snaplines);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Aimbot")) {
			ImGui::Checkbox("Enabled", &aimbot.enabled);
			ImGui::Checkbox("FOV limit", &aimbot.fovLimit);
			if (aimbot.fovLimit)
				ImGui::SliderFloat("FOV", &aimbot.fov, 1.f, 180.f, "%.0f");
			ImGui::Checkbox("Smoothing", &aimbot.smoothing);
			if (aimbot.smoothing)
				ImGui::SliderFloat("Smooth", &aimbot.smooth, 1.f, 20.f, "%.1f");
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

}
