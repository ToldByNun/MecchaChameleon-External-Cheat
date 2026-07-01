#pragma once

#include "Presets.hpp"

namespace Custom {

bool ColorPicker(
	const char* label,
	ImVec4* color,
	const ColorPickerPreset& preset = g_presets.colorPicker
);

} // namespace Custom
