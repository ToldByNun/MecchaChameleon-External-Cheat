#include "MainGui.hpp"

namespace Custom {

bool BeginMainGui(const char* id, bool* open, const MainGuiPreset& preset) {
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, preset.rounding);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

	const bool opened = ImGui::Begin(id, open, preset.windowFlags);
	if (!opened) {
		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(2);
		return false;
	}

	const ImVec2 pos = ImGui::GetWindowPos();
	const ImVec2 size = ImGui::GetWindowSize();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	drawList->AddRectFilled(
		pos,
		ImVec2(pos.x + size.x, pos.y + size.y),
		ColorToU32(preset.background)
	);

	if (preset.outlineSize > 0.f) {
		drawList->AddRect(
			pos,
			ImVec2(pos.x + size.x, pos.y + size.y),
			ColorToU32(preset.outline),
			preset.rounding,
			0,
			preset.outlineSize
		);
	}

	ImGui::SetCursorPos(ImVec2(0.f, 0.f));
	return true;
}

void EndMainGui() {
	ImGui::End();
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(2);
}

float GetContentHeight(float topBarHeight, float footerHeight) {
	return ImGui::GetWindowHeight() - topBarHeight - footerHeight;
}

} // namespace Custom
