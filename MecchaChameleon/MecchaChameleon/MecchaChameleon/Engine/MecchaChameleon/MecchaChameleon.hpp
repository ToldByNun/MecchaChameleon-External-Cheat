#ifndef MECCHACHAMELEON_HPP
#define	MECCHACHAMELEON_HPP

#include "../Memory/Memory.hpp"
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

struct TrackedActor {
    FVector location;
    double height;
};

class MecchaChameleon {
public:
	~MecchaChameleon();

	bool init();
	bool update();
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
};

#endif // MECCHACHAMELEON_HPP

