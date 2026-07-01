#pragma once

#include "Presets.hpp"

namespace Custom {

constexpr unsigned int kMultiComboMaxItems = 32;

constexpr unsigned int MultiComboBit(int index) {
	return 1u << index;
}

inline bool HasMultiComboFlag(unsigned int flags, int index) {
	return (flags & MultiComboBit(index)) != 0;
}

inline bool HasMultiComboFlag(unsigned int flags, unsigned int flag) {
	return (flags & flag) != 0;
}

// Each item index maps to bit (1 << index). Max 32 items.
bool MultiCombo(
	const char* label,
	unsigned int* flags,
	const char* const* items,
	int itemCount,
	const MultiComboPreset& preset = g_presets.multiCombo
);

} // namespace Custom
