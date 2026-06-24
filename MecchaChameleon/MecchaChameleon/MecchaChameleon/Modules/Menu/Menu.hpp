#ifndef MENU_HPP
#define MENU_HPP

#include "../../Manager/Classmanager/Classmanager.hpp"
#include <Windows.h>

class Menu : public IManagedClass {
public:
	void handleInput();
	void render();
};

#endif // MENU_HPP
