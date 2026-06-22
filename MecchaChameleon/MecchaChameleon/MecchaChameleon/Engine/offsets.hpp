#ifndef OFFSETS_HPP
#define OFFSETS_HPP

#include <cstdint>

#include <vector>

namespace Offsets {
	const uintptr_t GWorld = 0x9C6D620;

	struct SWorld {
		static constexpr uintptr_t PersistentLevel = 0x30;

		struct SLevel {
			static constexpr uintptr_t Actors = 0xA0;

			struct SActor {
				static constexpr uintptr_t Class = 0x10;
				static constexpr uintptr_t Name = 0x18;
			};
		};
	};
}

#endif OFFSETS_HPP