#pragma once
#include "overlay.h"
#include "driver.h"
#include "struct.h"
#include <atomic>

Driver* game = new Driver();

bool Crosshair = false;
bool esp = false;
bool espLine = false;
bool distanceESp = false;

bool showmenu = true;
std::atomic<bool> rendering(true);
std::atomic<bool> shouldExit(false);
FOverlay* g_overlay;

float ScreenCenterX;
float ScreenCenterY;
float distanceMax = 1000;

uint64_t process_base = 0;

// ========================================
// MECCHA CHAMELEON (UE5 5.6.1) Offsets
// ========================================
namespace Offsets {
    constexpr uintptr_t GWorld = 0x9C70620;
    constexpr uintptr_t GNames = 0x9E43080;  // Real FNamePool (unk_149E43080)

    constexpr uintptr_t UWorld_PersistentLevel = 0x30;
    constexpr uintptr_t UWorld_OwningGameInstance = 0x228;
    constexpr uintptr_t UWorld_GameState = 0x1B0;

    constexpr uintptr_t UGameInstance_LocalPlayers = 0x38;
    constexpr uintptr_t UPlayer_PlayerController = 0x30;

    constexpr uintptr_t APlayerController_AcknowledgedPawn = 0x350;
    constexpr uintptr_t APlayerController_PlayerCameraManager = 0x360;

    constexpr uintptr_t AController_PlayerState = 0x2B0;
    constexpr uintptr_t AController_Pawn = 0x2E8;

    constexpr uintptr_t APlayerCameraManager_CameraCache = 0x1530;
    constexpr uintptr_t FCameraCacheEntry_POV = 0x10;

    constexpr uintptr_t FMinimalViewInfo_Location = 0x0;
    constexpr uintptr_t FMinimalViewInfo_Rotation = 0x18;
    constexpr uintptr_t FMinimalViewInfo_FOV = 0x30;

    constexpr uintptr_t ULevel_Actors = 0xA0;
    constexpr uintptr_t ULevel_ActorCount = 0xA8;

    constexpr uintptr_t AActor_RootComponent = 0x1B8;
    constexpr uintptr_t AActor_Instigator = 0x1A0;
    constexpr uintptr_t UObject_Name = 0x18;
    constexpr uintptr_t APawn_PlayerState = 0x2C8;
    constexpr uintptr_t USceneComponent_RelativeLocation = 0x140;
    constexpr uintptr_t USceneComponent_ComponentToWorld = 0x100; // FTransform in pad area

    // ACharacter
    constexpr uintptr_t ACharacter_Mesh = 0x328; // USkeletalMeshComponent*

    // USkeletalMeshComponent (bone transforms)
    constexpr uintptr_t CachedComponentSpaceTransforms = 0x9B8; // TArray<FTransform>

    constexpr int FTransformSize = 0x60;

    // BP_FirstPersonCharacter_Main (parent class)
    constexpr uintptr_t Health = 0x640;           // double
    constexpr uintptr_t MaxHealthValue = 0x648;   // double

    // BP_FirstPersonCharacter_cLeon_Character (cLeon subclass)
    constexpr uintptr_t IsHunter = 0xC2A;         // bool
    constexpr uintptr_t IsLiveSelf = 0xC2C;       // bool
}
