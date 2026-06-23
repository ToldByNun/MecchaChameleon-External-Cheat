#include <iostream>
#include <thread>
#include <chrono>
#include <Windows.h>

#include "Engine/MecchaChameleon/MecchaChameleon.hpp"
#include "Modules/Overlay/Overlay.hpp"
#include "Modules/Menu/Menu.hpp"
#include "Modules/ESP/ESP.hpp"

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

	ESP esp;
	Menu menu;

	std::cout << "[+] Overlay running. Press INSERT to toggle menu.\n";

	mecchaChameleon.startBackgroundUpdate();

	const double targetFrameTime = 1.0 / 240.0;
	auto lastEspLog = std::chrono::steady_clock::now();

	while (overlay.isRunning()) {
		auto frameStart = std::chrono::high_resolution_clock::now();

		overlay.syncPosition();
		if (!overlay.processMessages())
			break;

		menu.handleInput();
		overlay.setClickThrough(!settings::menuOpen);

		std::vector<TrackedActor> actors;
		FMinimalViewInfo viewInfo{};
		mecchaChameleon.getSnapshot(actors, viewInfo);

		overlay.beginFrame();
		menu.render();

		if (settings::esp.snaplines) {
			if (!actors.empty())
				esp.renderSnaplines(actors, viewInfo);
		}
		else {
			const auto now = std::chrono::steady_clock::now();
			if (now - lastEspLog >= std::chrono::seconds(2)) {
				lastEspLog = now;
				std::cout << "[main] Snaplines aus — im Menu aktivieren\n";
			}
		}

		overlay.endFrame();

		auto frameEnd = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = frameEnd - frameStart;
		if (elapsed.count() < targetFrameTime) {
			std::this_thread::sleep_for(std::chrono::duration<double>(targetFrameTime - elapsed.count()));
		}
	}

	mecchaChameleon.stopBackgroundUpdate();
	overlay.shutdown();
	return 0;
}
