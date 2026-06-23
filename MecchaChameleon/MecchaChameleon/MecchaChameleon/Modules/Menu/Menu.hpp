#ifndef MENU_HPP
#define MENU_HPP

#include <Windows.h>

namespace settings {
	extern bool menuOpen;

	struct EspSettings {
		bool box = false;
		bool skeleton = false;
		bool nameDistance = false;
		bool snaplines = false;
	};
	extern EspSettings esp;

	struct AimbotSettings {
		bool enabled = false;
		bool fovLimit = false;
		bool smoothing = false;
		float fov = 90.f;
		float smooth = 5.f;
	};
	extern AimbotSettings aimbot;
}

class Menu {
public:
	void handleInput();
	void render();
};

#endif // MENU_HPP
