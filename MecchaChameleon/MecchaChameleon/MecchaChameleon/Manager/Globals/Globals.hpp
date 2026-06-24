#pragma once

class ClassManager;
class MecchaChameleon;
class Overlay;
class Menu;
class ESP;

struct EspSettings {
    bool box = false;
    bool skeleton = false;
    bool nameDistance = false;
    bool snaplines = false;
    bool chineseHat = false;
};

struct AimbotSettings {
    bool enabled = false;
    bool fovLimit = false;
    bool smoothing = false;
    float fov = 90.f;
    float smooth = 5.f;
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
    AppSettings settings{};
};

extern Globals globals;
