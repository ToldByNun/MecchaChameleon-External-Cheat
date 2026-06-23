#include <iostream>
#include <thread>
#include <chrono>
#include <Windows.h>

#include "Engine/MecchaChameleon/MecchaChameleon.hpp"
#include "Modules/Overlay/Overlay.hpp"
#include "Modules/Menu/Menu.hpp"

int main() {
	MecchaChameleon mecchaChameleon;
	if (!mecchaChameleon.init()) return 1;

	if (!mecchaChameleon.memory.windowHandle) {
		std::cout << "[-] No game window — cannot create overlay.\n";
		return 1;
	}

	Overlay overlay;
	if (!overlay.init(mecchaChameleon.memory.windowHandle)) {
		std::cout << "[-] Overlay init failed.\n";
		return 1;
	}

	std::cout << "[+] Overlay running. Press INSERT to toggle menu.\n";

	const double targetFrameTime = 1.0 / 240.0;

	while (overlay.isRunning()) {
		auto frameStart = std::chrono::high_resolution_clock::now();

		overlay.syncPosition();
		if (!overlay.processMessages())
			break;

		Menu::handleInput();
		overlay.setClickThrough(!Menu::isOpen());

		overlay.beginFrame();
		Menu::render();
		overlay.endFrame();

		if (mecchaChameleon.actors.count > 0) {
			// ESP / actor logic goes here
		}

		auto frameEnd = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = frameEnd - frameStart;
		if (elapsed.count() < targetFrameTime) {
			std::this_thread::sleep_for(std::chrono::duration<double>(targetFrameTime - elapsed.count()));
		}
	}

	overlay.shutdown();
	return 0;
}
