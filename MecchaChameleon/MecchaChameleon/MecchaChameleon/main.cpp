#include <Windows.h>
#include <iostream>

#include "Engine/Memory/Memory.hpp"

int main() {
	Memory memory;
	if (!memory.attachToProcess("PenguinHotel-Win64-Shipping.exe")) {
		std::cout << "Failed to attach to process." << std::endl;
		return 0;
	}

	std::cout << "Successfully attached to process." << std::endl;
	memory.detachFromProcess();

	return 0;
}