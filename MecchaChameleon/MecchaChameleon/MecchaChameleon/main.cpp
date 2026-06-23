#include <iostream>
#include <thread>
#include <chrono>
#include <Windows.h>

#include "Engine/MecchaChameleon/MecchaChameleon.hpp"

int main() {
	MecchaChameleon mecchaChameleon;
	if (!mecchaChameleon.init()) return 1;

    const double targetFrameTime = 1.0 / 60.0;

    while (true) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) break;
        }

        if (mecchaChameleon.actors.count <= 0) continue;

        // Test ESP soon

        auto frameEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = frameEnd - frameStart;
        if (elapsed.count() < targetFrameTime) {
            std::this_thread::sleep_for(std::chrono::duration<double>(targetFrameTime - elapsed.count()));
        }
    }
	return 0;
}