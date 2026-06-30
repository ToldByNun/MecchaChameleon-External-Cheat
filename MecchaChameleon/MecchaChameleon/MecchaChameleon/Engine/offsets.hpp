#ifndef OFFSETS_HPP
#define OFFSETS_HPP

#include <cstdint>

#include <vector>

namespace Offsets {
    namespace AOBs {
        inline constexpr const char* GWorldAOB = "44 38 2D ?? ?? ?? ?? 48 8B 1D ?? ?? ?? ?? 74 ?? 48 85 DB 74 ?? 48 8B CB E8 ?? ?? ?? ??";
        inline constexpr uintptr_t GWorldInstructionOffset = 0x7;

        inline constexpr const char* GNamesAOB = "80 3D ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? E9 ?? ?? ?? ??";
        inline constexpr uintptr_t GNamesInstructionOffset = 0xD;
    }

    inline uintptr_t GWorld = 0;
    inline uintptr_t GNames = 0;

    struct SWorld {
        static constexpr uintptr_t PersistentLevel = 0x30;
        static constexpr uintptr_t OwningGameInstance = 0x228;
        static constexpr uintptr_t GameState = 0x1B0;

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
                    static constexpr uintptr_t AcknowledgedPawn = 0x350;
                    static constexpr uintptr_t PlayerCameraManager = 0x360;
                    static constexpr uintptr_t LocalPawn = 0x2E8;

                    struct SPawn {
                        static constexpr uintptr_t CharacterMovementComponent = 0x1530;

                        struct SCharacterMovementComponent {
                            static constexpr uintptr_t MaxWalkSpeed = 0x278;
                            static constexpr uintptr_t MaxAcceleration = 0x28C;
                            static constexpr uintptr_t MinAnalogWalkSpeed = 0x290;
                        };
                    };

                    struct SPlayerCameraManager {
                        static constexpr uintptr_t CameraCachePrivate = 0x1530;
                        static constexpr uintptr_t CameraInfo = 0x1540;
                        static constexpr uintptr_t CameraPOV = 0x10;
                    };
                };
            };
        };

        struct SGameState {
            static constexpr uintptr_t PlayerArray = 0x2C0;

            struct SPlayerArray {
                static constexpr uintptr_t Pawn = 0x320;
                static constexpr uintptr_t PlayerName = 0x340;

                struct SPawn {
                    static constexpr uintptr_t Mesh = 0x418;
                    static constexpr uintptr_t HeadPosition = 0x400;

                    struct SMesh {
                        static constexpr uintptr_t OverrideMaterials = 0x520;

                        struct SMaterial {
                            static constexpr uintptr_t ParentMaterial = 0x128;
                        };
                    };

                    struct SHeadPosition {
                        static constexpr uintptr_t HeadRadius = 0x540;
                        static constexpr uintptr_t PlayerSize = 0x130;
                    };
                };

                struct SMesh {
                    static constexpr uintptr_t Class = 0x10;
                    static constexpr uintptr_t SkeletalMesh = 0x578;
                    static constexpr uintptr_t BoneSpaceTransforms = 0x9A8;
                    static constexpr uintptr_t ComponentSpaceTransforms = 0x9B8;
                };
            };
        };
    };
}

#endif // OFFSETS_HPP
