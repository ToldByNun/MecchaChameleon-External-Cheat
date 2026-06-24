#ifndef MECCHACHAMELEON_HPP
#define MECCHACHAMELEON_HPP

#include "../../Manager/Classmanager/Classmanager.hpp"
#include "../Memory/Memory.hpp"
#include "../ImGui/imgui.h"
#include "../offsets.hpp"
#include "../types.hpp"
#include "../helpers.hpp"

#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <iomanip>
#include <iostream>
#include <unordered_set>

struct TrackedActor {
    FVector location;
    double headRadius;
    double playerSize;
    std::string playerName;
};

class MecchaChameleon : public IManagedClass {
public:
	~MecchaChameleon();

	bool init() override;
	void deinit() override;
	void update() override;
	void getSnapshot(std::vector<TrackedActor>& outActors, FMinimalViewInfo& outViewInfo);
	void startBackgroundUpdate();
	void stopBackgroundUpdate();

public:
    Memory memory;
    Helpers helpers;

private:
    uintptr_t world = 0;
    uintptr_t names = 0;
    uintptr_t persistentLevel = 0;
    uintptr_t gameInstance = 0;
    uintptr_t localPlayer = 0;
    uintptr_t playerController = 0;
    uintptr_t cameraManager = 0;
    uintptr_t gameState = 0;

    std::vector<TrackedActor> actors;
    FMinimalViewInfo viewInfo{};
    std::mutex dataMutex;
    std::atomic<bool> backgroundRunning{ false };
    std::thread updateThread;

    bool chainResolved = false;
    void updateLoop();

    std::string resolveName(uint32_t index) {
        return helpers.resolveName(memory, index);
    }

    std::string getNameByPtr(uintptr_t actorPtr) {
        int32_t fnameID = memory.readMemory<int32_t>(actorPtr + Offsets::SWorld::SLevel::SActor::Name);

        return resolveName(fnameID);
    }

	bool resolveChain();
	bool resolvePersistentLevel();
	bool resolveGameInstance();
	bool resolveLocalPlayer();
	bool resolvePlayerController();
	bool resolveCameraManager();
	bool resolveGameState();
	bool validatePlayerArray();
    bool refresh();

    template <typename T, typename = void> struct is_tarray : std::false_type {};
    template <typename T> struct is_tarray<T, std::void_t<decltype(std::declval<T>().data), decltype(std::declval<T>().count)>> : std::true_type {};

    template <typename T, typename = void> struct is_fvector : std::false_type {};
    template <typename T> struct is_fvector<T, std::void_t<decltype(std::declval<T>().x), decltype(std::declval<T>().y), decltype(std::declval<T>().z)>> : std::true_type {};

    template <typename T, typename = void> struct is_fname : std::false_type {};
    template <typename T> struct is_fname<T, std::void_t<decltype(std::declval<T>().comparisonIndex)>> : std::true_type {};

    template <typename T>
    bool check(const T& val, const std::string& name) {
        bool valid = false;

        std::string extraInfo = "";

        if constexpr (is_tarray<T>::value) {
            valid = (val.data != 0 && val.count >= 0 && val.count < 1000000);
            if (valid) extraInfo = "[Count: " + std::to_string(val.count) + "] at 0x" + std::to_string(val.data);
        }
        else if constexpr (is_fvector<T>::value) {
            valid = (val.x != 0 || val.y != 0 || val.z != 0);
            extraInfo = "X: " + std::to_string((int)val.x) + " Y: " + std::to_string((int)val.y) + " Z: " + std::to_string((int)val.z);
        }
        else if constexpr (is_fname<T>::value) {
            valid = (val.comparisonIndex > 0);
            extraInfo = "Index: " + std::to_string(val.comparisonIndex);
        }
        else {
            if constexpr (std::is_integral_v<T> || std::is_pointer_v<T>) {
                valid = (val != 0);
            }
            else {
                valid = true;
            }
        }

        if (!valid) {
            std::cout << "\033[1;31m[-] " << std::setw(32) << std::left << name << " : INVALID\033[0m" << std::endl;
            return false;
        }

        std::cout << "\033[0m[+] " << std::setw(32) << std::left << name << " : ";

        if constexpr (std::is_integral_v<T> || std::is_pointer_v<T>) {
            std::cout << "0x" << std::hex << std::uppercase << (uintptr_t)val << std::dec;
        }
        else {
            std::cout << "SUCCESS " << extraInfo;
        }

        std::cout << "\033[0m" << std::endl;
        return true;
    }

public:
    void dumpObjectRefsDeep(
        uintptr_t object,
        size_t scanSize = 0x800,
        int depth = 1,
        int indent = 0,
        size_t maxNameLength = 64,
        bool skipNone = true,
        std::unordered_set<uintptr_t>* visitedPtr = nullptr
    ) {
        if (!object || depth < 0)
            return;

        std::unordered_set<uintptr_t> localVisited;
        if (!visitedPtr)
            visitedPtr = &localVisited;

        if (visitedPtr->contains(object))
            return;

        visitedPtr->insert(object);

        std::string pad(indent * 2, ' ');

        std::string objName = getNameByPtr(object);
        uintptr_t objClassPtr = memory.readMemory<uintptr_t>(object + 0x10);
        std::string objClass = getNameByPtr(objClassPtr);

        std::cout << pad << objName << " class=" << objClass
            << " [0x" << std::hex << object << std::dec << "]\n";

        if (depth == 0)
            return;

        for (uintptr_t off = 0; off < scanSize; off += 0x8) {
            uintptr_t ptr = memory.readMemory<uintptr_t>(object + off);
            if (!ptr || ptr == object)
                continue;

            uintptr_t cls = memory.readMemory<uintptr_t>(ptr + 0x10);
            std::string clsName = getNameByPtr(cls);
            std::string name = getNameByPtr(ptr);

            if (skipNone && (name.empty() || name == "None" || clsName.empty() || clsName == "None" || clsName == "TextureRenderTarget2D" || clsName == "Class" || clsName == "Texture2D" || clsName == "MaterialInstanceConstant" || clsName == "Function" || clsName == "Model" || clsName == "AnimBlueprintGeneratedClass" || clsName == "BlueprintGeneratedClass" || clsName == "NiagaraDataInterfaceVectorField" || clsName == "HorizontalBoxSlot"))
                continue;

            if (name.length() > maxNameLength || clsName.length() > maxNameLength)
                continue;

            std::cout << pad
                << "  +0x" << std::hex << off << std::dec
                << " -> 0x" << std::hex << ptr << std::dec
                << " " << name
                << " class=" << clsName
                << "\n";

            dumpObjectRefsDeep(ptr, scanSize, depth - 1, indent + 1, maxNameLength, skipNone, visitedPtr);
        }
    }

    void dumpFStrings(uintptr_t object, size_t scanSize = 0x800) {
        for (uintptr_t off = 0; off < scanSize; off += 0x8) {
            uintptr_t data = memory.readMemory<uintptr_t>(object + off);
            int32_t count = memory.readMemory<int32_t>(object + off + 0x8);
            int32_t max = memory.readMemory<int32_t>(object + off + 0xC);

            if (!data || count <= 0 || count > 64 || max < count || max > 128)
                continue;

            std::wstring value(count, L'\0');

            if (!memory.readRawMemory(data, value.data(), count * sizeof(wchar_t)))
                continue;

            bool valid = true;
            for (wchar_t c : value) {
                if (c == L'\0')
                    continue;

                if (c < 32 || c > 126) {
                    valid = false;
                    break;
                }
            }

            if (!valid)
                continue;

            std::wcout
                << L"+0x" << std::hex << off << std::dec
                << L" FString = " << value << L"\n";
        }
    }

};

#endif // MECCHACHAMELEON_HPP
