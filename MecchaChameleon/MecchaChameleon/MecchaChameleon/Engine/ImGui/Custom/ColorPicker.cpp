#include "ColorPicker.hpp"

#include "../imgui_internal.h"

#include <algorithm>
#include <cmath>

namespace Custom {

namespace {
	float Clamp(float value, float min, float max) {
		return std::max(min, std::min(max, value));
	}

	void RgbToHsv(const ImVec4& color, float& h, float& s, float& v) {
		const float r = color.x;
		const float g = color.y;
		const float b = color.z;

		const float maxValue = std::max(r, std::max(g, b));
		const float minValue = std::min(r, std::min(g, b));
		const float delta = maxValue - minValue;

		v = maxValue;
		s = (maxValue <= 0.f) ? 0.f : delta / maxValue;

		if (delta <= 0.f) {
			h = 0.f;
			return;
		}

		if (maxValue == r) {
			h = (g - b) / delta + (g < b ? 6.f : 0.f);
		}
		else if (maxValue == g) {
			h = (b - r) / delta + 2.f;
		}
		else {
			h = (r - g) / delta + 4.f;
		}

		h /= 6.f;
	}

	ImVec4 HsvToRgb(float h, float s, float v, float alpha) {
		if (s <= 0.f)
			return ImVec4(v, v, v, alpha);

		h = h - std::floor(h);
		const float hueSector = h * 6.f;
		const int sector = static_cast<int>(hueSector);
		const float fraction = hueSector - static_cast<float>(sector);

		const float p = v * (1.f - s);
		const float q = v * (1.f - s * fraction);
		const float t = v * (1.f - s * (1.f - fraction));

		switch (sector % 6) {
		case 0: return ImVec4(v, t, p, alpha);
		case 1: return ImVec4(q, v, p, alpha);
		case 2: return ImVec4(p, v, t, alpha);
		case 3: return ImVec4(p, q, v, alpha);
		case 4: return ImVec4(t, p, v, alpha);
		default: return ImVec4(v, p, q, alpha);
		}
	}

	void DrawGrab(ImDrawList* drawList, const ImVec2& center, float radius, ImU32 fillColor, ImU32 borderColor) {
		drawList->AddCircleFilled(center, radius, fillColor);
		drawList->AddCircle(center, radius, borderColor, 0, 1.f);
	}
}

bool ColorPicker(const char* label, ImVec4* color, const ColorPickerPreset& preset) {
	if (!color)
		return false;

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	const ImGuiID id = window->GetID(label);
	const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);
	const float availWidth = ImGui::GetContentRegionAvail().x;
	const float rowHeight = preset.rowHeight;

	const ImVec2 pos = window->DC.CursorPos;
	const ImRect totalBb(pos, ImVec2(pos.x + availWidth, pos.y + rowHeight));
	ImGui::ItemSize(totalBb);
	if (!ImGui::ItemAdd(totalBb, id))
		return false;

	const float previewX = pos.x + availWidth - preset.previewSize;
	const float previewY = pos.y + (rowHeight - preset.previewSize) * 0.5f;
	const ImRect previewBb(
		ImVec2(previewX, previewY),
		ImVec2(previewX + preset.previewSize, previewY + preset.previewSize)
	);

	bool hovered = false;
	bool held = false;
	const bool pressed = ImGui::ButtonBehavior(previewBb, id, &hovered, &held);

	ImDrawList* drawList = window->DrawList;

	const float labelY = pos.y + (rowHeight - labelSize.y) * 0.5f;
	drawList->AddText(ImVec2(pos.x, labelY), ColorToU32(preset.labelColor), label);

	const ImVec4 previewBorder = hovered ? preset.previewBorderHovered : preset.previewBorder;
	drawList->AddRectFilled(
		previewBb.Min,
		previewBb.Max,
		ImGui::ColorConvertFloat4ToU32(ImVec4(color->x, color->y, color->z, 1.f)),
		preset.rounding
	);
	drawList->AddRect(
		previewBb.Min,
		previewBb.Max,
		ColorToU32(previewBorder),
		preset.rounding,
		0,
		1.f
	);

	bool changed = false;

	ImGui::PushID(id);
	if (pressed)
		ImGui::OpenPopup("##color_picker_popup");

	ImGui::SetNextWindowPos(ImVec2(previewBb.Min.x, previewBb.Max.y + 4.f), ImGuiCond_Appearing);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(preset.popupPadding, preset.popupPadding));
	ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, preset.rounding);
	ImGui::PushStyleColor(ImGuiCol_PopupBg, preset.popupBackground);
	ImGui::PushStyleColor(ImGuiCol_Border, preset.popupBorder);

	if (ImGui::BeginPopup("##color_picker_popup", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
		float hue = 0.f;
		float saturation = 0.f;
		float value = 0.f;
		RgbToHsv(*color, hue, saturation, value);

		ImGuiWindow* popupWindow = ImGui::GetCurrentWindow();
		ImDrawList* popupDrawList = popupWindow->DrawList;
		const ImVec2 pickerPos = popupWindow->DC.CursorPos;
		const ImVec2 svMax(
			pickerPos.x + preset.svPickerSize,
			pickerPos.y + preset.svPickerSize
		);
		const ImRect svBb(pickerPos, svMax);

		const ImGuiID svId = popupWindow->GetID("##sv");
		const ImGuiID hueId = popupWindow->GetID("##hue");

		ImGui::ItemSize(svBb);
		ImGui::ItemAdd(svBb, svId);

		const ImVec4 hueColor = HsvToRgb(hue, 1.f, 1.f, 1.f);
		popupDrawList->AddRectFilledMultiColor(
			svBb.Min,
			svBb.Max,
			IM_COL32(255, 255, 255, 255),
			ImGui::ColorConvertFloat4ToU32(hueColor),
			IM_COL32(0, 0, 0, 255),
			IM_COL32(0, 0, 0, 255)
		);
		popupDrawList->AddRect(
			svBb.Min,
			svBb.Max,
			ColorToU32(preset.popupBorder),
			preset.rounding,
			0,
			1.f
		);

		const ImVec2 huePos(svBb.Min.x, svBb.Max.y + preset.popupPadding);
		const ImVec2 hueMax(svBb.Max.x, huePos.y + preset.hueBarHeight);
		const ImRect hueBb(huePos, hueMax);

		ImGui::SetCursorScreenPos(huePos);
		ImGui::ItemSize(hueBb);
		ImGui::ItemAdd(hueBb, hueId);

		constexpr int hueSegments = 6;
		const float segmentWidth = hueBb.GetWidth() / static_cast<float>(hueSegments);
		const ImU32 hueColors[hueSegments + 1] = {
			IM_COL32(255, 0, 0, 255),
			IM_COL32(255, 255, 0, 255),
			IM_COL32(0, 255, 0, 255),
			IM_COL32(0, 255, 255, 255),
			IM_COL32(0, 0, 255, 255),
			IM_COL32(255, 0, 255, 255),
			IM_COL32(255, 0, 0, 255)
		};

		for (int segment = 0; segment < hueSegments; ++segment) {
			const float x0 = hueBb.Min.x + segmentWidth * static_cast<float>(segment);
			const float x1 = x0 + segmentWidth;
			popupDrawList->AddRectFilledMultiColor(
				ImVec2(x0, hueBb.Min.y),
				ImVec2(x1, hueBb.Max.y),
				hueColors[segment],
				hueColors[segment + 1],
				hueColors[segment + 1],
				hueColors[segment]
			);
		}

		popupDrawList->AddRect(
			hueBb.Min,
			hueBb.Max,
			ColorToU32(preset.popupBorder),
			preset.rounding,
			0,
			1.f
		);

		const ImVec2 mousePos = ImGui::GetIO().MousePos;
		const bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
		const bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

		if (mouseClicked && ImGui::IsMouseHoveringRect(svBb.Min, svBb.Max, false))
			ImGui::SetActiveID(svId, popupWindow);
		if (mouseClicked && ImGui::IsMouseHoveringRect(hueBb.Min, hueBb.Max, false))
			ImGui::SetActiveID(hueId, popupWindow);

		if (ImGui::GetActiveID() == svId && mouseDown) {
			const float newSaturation = Clamp((mousePos.x - svBb.Min.x) / svBb.GetWidth(), 0.f, 1.f);
			const float newValue = 1.f - Clamp((mousePos.y - svBb.Min.y) / svBb.GetHeight(), 0.f, 1.f);
			const ImVec4 newColor = HsvToRgb(hue, newSaturation, newValue, color->w);

			if (newColor.x != color->x || newColor.y != color->y || newColor.z != color->z) {
				*color = newColor;
				changed = true;
			}

			saturation = newSaturation;
			value = newValue;
		}
		else if (ImGui::GetActiveID() == hueId && mouseDown) {
			const float newHue = Clamp((mousePos.x - hueBb.Min.x) / hueBb.GetWidth(), 0.f, 1.f);
			const ImVec4 newColor = HsvToRgb(newHue, saturation, value, color->w);

			if (newColor.x != color->x || newColor.y != color->y || newColor.z != color->z) {
				*color = newColor;
				changed = true;
			}

			hue = newHue;
		}

		if (!mouseDown && (ImGui::GetActiveID() == svId || ImGui::GetActiveID() == hueId))
			ImGui::ClearActiveID();

		const ImVec2 svGrab(
			svBb.Min.x + saturation * svBb.GetWidth(),
			svBb.Min.y + (1.f - value) * svBb.GetHeight()
		);
		const ImVec2 hueGrab(
			hueBb.Min.x + hue * hueBb.GetWidth(),
			hueBb.Min.y + hueBb.GetHeight() * 0.5f
		);

		DrawGrab(
			popupDrawList,
			svGrab,
			preset.grabRadius,
			IM_COL32(255, 255, 255, 255),
			ColorToU32(preset.grabBorder)
		);
		DrawGrab(
			popupDrawList,
			hueGrab,
			preset.grabRadius - 1.f,
			IM_COL32(255, 255, 255, 255),
			ColorToU32(preset.grabBorder)
		);

		ImGui::SetCursorScreenPos(ImVec2(svBb.Min.x, hueBb.Max.y));
		ImGui::Dummy(ImVec2(svBb.GetWidth(), 0.f));

		ImGui::EndPopup();
	}

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);
	ImGui::PopID();

	return changed;
}

} // namespace Custom
