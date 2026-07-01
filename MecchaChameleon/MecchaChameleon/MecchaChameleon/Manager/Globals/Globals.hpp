#pragma once

class ClassManager;
class MecchaChameleon;
class Overlay;
class Menu;
class ESP;
class Aimbot;

struct EspSettings {
    bool box = false;
    bool corners = false;
    bool dynamicBoxes = false;
    bool skeleton = true;
    bool name = false;
    bool distance = false;
    bool snaplines = false;
    bool chineseHat = false;
    bool fovCircle = false;
    bool onlyEnemies = false;
    bool isTeammateColorEnabled = false;

    bool devMode = true;
};

struct AimbotSettings {
    bool enabled = false;
    bool fovLimit = false;
    bool smoothing = false;
    float fov = 90.f;
    float smooth = 5.f;
    int keybind = 2; // WinUser.h:465 - RightClick
};

struct AppSettings {
    bool menuOpen = true;
    EspSettings esp{};
    AimbotSettings aimbot{};
};

struct Globals {
    ClassManager* classManager = nullptr;
    MecchaChameleon* mecchaChameleon = nullptr;
    Overlay* overlay = nullptr;
    Menu* menu = nullptr;
    ESP* esp = nullptr;
    Aimbot* aimbot = nullptr;
    AppSettings settings{};
};

extern Globals globals;
