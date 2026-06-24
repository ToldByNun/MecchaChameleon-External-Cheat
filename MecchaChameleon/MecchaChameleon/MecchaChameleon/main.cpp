#include <iostream>
#include <thread>
#include <chrono>
#include <Windows.h>

#include "Manager/Classmanager/Classmanager.hpp"
#include "Manager/Globals/Globals.hpp"
#include "Engine/MecchaChameleon/MecchaChameleon.hpp"
#include "Modules/Overlay/Overlay.hpp"
#include "Modules/Menu/Menu.hpp"
#include "Modules/ESP/ESP.hpp"

int main() {
	ClassManager classManager;
	globals = {};
	globals.classManager = &classManager;
	classManager.setState(&globals);

	globals.mecchaChameleon = classManager.addClass<MecchaChameleon>("mecchaChameleon");
	globals.menu = classManager.addClass<Menu>("menu");
	globals.esp = classManager.addClass<ESP>("esp");
	globals.overlay = classManager.addClass<Overlay>("overlay");

	if (!classManager.init()) {
		globals = {};
		return 1;
	}

	if (!globals.mecchaChameleon || !globals.menu || !globals.esp || !globals.overlay)
		return 1;

	if (!globals.mecchaChameleon->memory.windowHandle) {
		std::cout << "[-] No game window - cannot create overlay.\n";
		classManager.deinit();
		globals = {};
		return 1;
	}

	std::cout << "[+] Overlay running. Press INSERT to toggle menu.\n";

	globals.mecchaChameleon->startBackgroundUpdate();

	const double targetFrameTime = 1.0 / 240.0;

	while (globals.overlay->isRunning()) {
		auto frameStart = std::chrono::high_resolution_clock::now();

		globals.overlay->syncPosition();
		if (!globals.overlay->processMessages())
			break;

		globals.menu->handleInput();
		globals.overlay->setClickThrough(!globals.settings.menuOpen);

		std::vector<TrackedActor> actors;
		FMinimalViewInfo viewInfo{};
		globals.mecchaChameleon->getSnapshot(actors, viewInfo);

		globals.overlay->beginFrame();
		globals.menu->render();

		if (globals.settings.esp.snaplines && !actors.empty())
			globals.esp->renderSnaplines(actors, viewInfo);

		globals.overlay->endFrame();

		auto frameEnd = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = frameEnd - frameStart;
		if (elapsed.count() < targetFrameTime) {
			std::this_thread::sleep_for(std::chrono::duration<double>(targetFrameTime - elapsed.count()));
		}
	}

	globals.mecchaChameleon->stopBackgroundUpdate();
	classManager.deinit();
	globals = {};
	return 0;
}
