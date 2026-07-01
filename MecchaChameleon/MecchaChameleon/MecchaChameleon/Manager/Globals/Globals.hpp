#pragma once

#include "../../Engine/ImGui/Custom/ColorPicker.hpp"

class ClassManager;
class MecchaChameleon;
class Overlay;
class Menu;
class ESP;
class Aimbot;

struct EspSettings {
    bool fovCircle = false;
    bool box = false;
    bool corners = false;
    ImVec4 defaultBoxColor = Custom::ColorFromHex(0xFFFFFF);
    ImVec4 enemyBoxColor = Custom::ColorFromHex(0x9966ff);
    bool dynamicBoxes = false;
    bool skeleton = false;
    ImVec4 defaultSkeletonColor = Custom::ColorFromHex(0xFFFFFF);
    ImVec4 enemySkeletonColor = Custom::ColorFromHex(0xFFFFFF);
    bool chineseHat = false;
    bool name = false;
    bool distance = false;
    bool snaplines = false;
    ImVec4 defaultSnaplineColor = Custom::ColorFromHex(0xFFFFFF);
    ImVec4 enemySnaplineColor = Custom::ColorFromHex(0x9966ff);
    bool minimap = false;

    // options
    int selectedTeam = 0;
    bool onlyEnemies = false;
    bool isTeammateColorEnabled = false;
    bool hideEnemiesMM = false;
    bool hideTeammatesMM = false;
    ImVec4 defaultMMColor = Custom::ColorFromHex(0xFFFFFF);
    ImVec4 enemyMMColor = Custom::ColorFromHex(0x9966ff);

    bool devMode = false;
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
