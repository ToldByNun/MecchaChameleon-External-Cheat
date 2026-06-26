#pragma once

#include "Presets.hpp"

namespace Custom {

bool SliderFloat(
	const char* label,
	float* value,
	float min,
	float max,
	const char* fmt = "%.1f",
	const SliderPreset& preset = g_presets.slider
);

} // namespace Custom
