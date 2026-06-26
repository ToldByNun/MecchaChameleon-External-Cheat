#pragma once

#include "Presets.hpp"

namespace Custom {

bool Toggle(const char* label, bool* value, const TogglePreset& preset = g_presets.toggle);

} // namespace Custom
