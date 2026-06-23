#ifndef MECCHACHAMELEON_HPP
#define	MECCHACHAMELEON_HPP

#include "../Memory/Memory.hpp"
#include "../types.hpp"
#include "../helpers.hpp"
#include <string>
#include <iomanip>
#include <iostream>

class MecchaChameleon {
public:
	bool init();
	bool update();

public:
    Memory memory;
    Helpers helpers;

    uintptr_t world = 0;
    uintptr_t persistentLevel = 0;

    TArray    actors = {};
    uintptr_t actor = 0;
    uintptr_t actorClass = 0;
    FName     className = {};

private:
    std::string resolveName(uint32_t index) {
        return helpers.resolveName(memory, memory.baseAddress, index);
    }

    std::string getNameByPtr(uintptr_t actorPtr) {
        int32_t fnameID = memory.readMemory<int32_t>(actorPtr + Offsets::SWorld::SLevel::SActor::Name);

        return resolveName(fnameID);
    }

	bool resolveChain();

    // Boilerplate reused from old project
    // Used to validate addresses
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

#endif MECCHACHAMELEON_HPP
