#pragma once

#include "Presets.hpp"

namespace Custom {

int TopBar(
	const char* title,
	const char* const* categories,
	int count,
	int* activeIndex,
	const TopBarPreset& preset = g_presets.topBar
);

} // namespace Custom
