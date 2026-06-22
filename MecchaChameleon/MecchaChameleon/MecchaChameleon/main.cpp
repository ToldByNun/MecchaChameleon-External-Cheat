#include <Windows.h>
#include <iostream>

#include "Engine/MecchaChameleon/MecchaChameleon.hpp"

int main() {
	MecchaChameleon mecchaChameleon;
	if (!mecchaChameleon.init()) return 1;

	return 0;
}