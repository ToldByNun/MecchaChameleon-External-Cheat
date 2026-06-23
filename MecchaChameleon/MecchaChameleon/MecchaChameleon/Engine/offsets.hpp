#ifndef OFFSETS_HPP
#define OFFSETS_HPP

#include <cstdint>

#include <vector>

namespace Offsets {
    const uintptr_t GWorld = 0xA0B21F0;
    const uintptr_t GNames = 0x9E40280;

    struct SWorld {
        static constexpr uintptr_t PersistentLevel = 0x30;
        static constexpr uintptr_t OwningGameInstance = 0x228;

        struct SLevel {
            static constexpr uintptr_t Actors = 0xA0;

            struct SActor {
                static constexpr uintptr_t Class = 0x10;
                static constexpr uintptr_t Name = 0x18;
                static constexpr uintptr_t RootComponent = 0x1B8;

                struct SComponent {
                    static constexpr uintptr_t RelativeLocation = 0x140;
                    static constexpr uintptr_t RelativeRotation = 0x158;
                    static constexpr uintptr_t RelativeScale3D = 0x170;
                };
            };
        };

        struct SGameInstance {
            static constexpr uintptr_t LocalPlayers = 0x38;

            struct SLocalPlayers {
                static constexpr uintptr_t PlayerController = 0x30;

                struct SPlayerController {
                    static constexpr uintptr_t PlayerCameraManager = 0x360;

                    struct SPlayerCameraManager {
                        static constexpr uintptr_t CameraCachePrivate = 0x1530;
                        static constexpr uintptr_t CameraInfo = 0x1540;
                        static constexpr uintptr_t CameraPOV = 0x10;
                    };
                };
            };
        };
    };
}

#endif // OFFSETS_HPP