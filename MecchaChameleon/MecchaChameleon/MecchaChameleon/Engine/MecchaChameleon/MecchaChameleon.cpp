#include "MecchaChameleon.hpp"
#include "../offsets.hpp"
#include <iostream>
#include <chrono>
#include <thread>

bool MecchaChameleon::init() {
	if (!memory.attachToProcess("PenguinHotel-Win64-Shipping.exe")) {
		std::cout << "[-] Failed to attach to process.\n";
		return false;
	}

	this->check(memory.baseAddress, "BaseAddress");

	while (!this->resolveChain()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return true;
}

bool MecchaChameleon::resolveChain() {
	this->world = memory.readMemory<uintptr_t>(memory.baseAddress + Offsets::GWorld);
	if (!this->check(this->world, "GWorld")) return false;

	return true;
}