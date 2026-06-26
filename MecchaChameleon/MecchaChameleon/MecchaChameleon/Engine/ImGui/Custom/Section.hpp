#pragma once

#include "Presets.hpp"

namespace Custom {

bool BeginSection(const char* label, ImVec2 size = ImVec2(0.f, 0.f), const SectionPreset& preset = g_presets.section);
void EndSection();

void BeginSectionDualLayout(float height, const SectionPreset& preset = g_presets.section);
bool BeginSectionDualLeft(const char* label, const SectionPreset& preset = g_presets.section);
void EndSectionDualLeft();
bool BeginSectionDualRight(const char* label, const SectionPreset& preset = g_presets.section);
void EndSectionDualRight();
void EndSectionDualLayout();

} // namespace Custom
