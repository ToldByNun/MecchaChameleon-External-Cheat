#pragma once

#include "Presets.hpp"

namespace Custom {

bool BeginMainGui(const char* id, bool* open, const MainGuiPreset& preset = g_presets.mainGui);
void EndMainGui();

float GetContentHeight(float topBarHeight, float footerHeight);

} // namespace Custom
