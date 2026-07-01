#pragma once

#include "Presets.hpp"

namespace Custom {

bool Dropdown(
	const char* label,
	int* currentIndex,
	const char* const* items,
	int itemCount,
	const DropdownPreset& preset = g_presets.dropdown
);

} // namespace Custom
