#pragma once
#include "global.h"
#include <cmath>
#include <vector>
#include <d3d9.h>

HWND hwnd = NULL;
HWND hwnd_active = NULL;
HWND OverlayWindow = NULL;
auto CrosshairColor = D2D1::ColorF(0, 100, 255, 255);
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================
// Structs
// ============================
struct FMinimalViewInfoRead {
    double Location[3];    // 0x00
    double Rotation[3];    // 0x18
    float FOV;             // 0x30
    float DesiredFOV;      // 0x34
    float FirstPersonFOV;  // 0x38
    float FirstPersonScale;// 0x3C
    float OrthoWidth;      // 0x40
    bool  bAutoCalc;       // 0x44
    uint8_t pad45[3];
    float AutoPlaneShift;  // 0x48
    bool  bUpdateOrtho;    // 0x4C
    bool  bUseCamHeight;   // 0x4D
    uint8_t pad4e[2];
    float OrthoNear;       // 0x50
    float OrthoFar;        // 0x54
    float PerspNear;       // 0x58
    float AspectRatio;     // 0x5C
};

struct CameraData {
    Vector3 Location;
    Vector3 Rotation;
    float FOV;
};

// UE5 FTransform (0x60 bytes)
struct FTransformRead {
    double Rotation[4];    // 0x00 FQuat (X,Y,Z,W)
    double Translation[3]; // 0x20 FVector
    double pad0;           // 0x38
    double Scale[3];       // 0x40 FVector
    double pad1;           // 0x58
};

// ============================
// Bone IDs (common UE mannequin skeleton)
// ============================
namespace Bone {
    enum : int {
        Head = 110,
        Neck = 109,
        Chest = 7,
        Pelvis = 2,
        UpperArmL = 37,
        LowerArmL = 38,
        HandL = 39,
        UpperArmR = 64,
        LowerArmR = 65,
        HandR = 66,
        ThighL = 93,
        CalfL = 94,
        FootL = 95,
        ThighR = 100,
        CalfR = 101,
        FootR = 102,
    };
}

// ============================
// Globals
// ============================
CameraData cameraData;
uintptr_t uWorld, gameInstance, persistentLevel;
uintptr_t localPlayerPtr, localPlayer, playerController;
uintptr_t pawn, cameraManager, actorsArray;
int actorsCount;
int debugPlayerCount = 0;

// ============================
// GNames lookup
// ============================
static std::string GetNameById(uint32_t actor_id) {
    char name[256] = { 0 };
    uint32_t block_idx = actor_id >> 16;
    uint32_t entry_idx = actor_id & 0xFFFF;
    uintptr_t pool = process_base + Offsets::GNames;
    uintptr_t block_ptr = game->rpm<uintptr_t>(pool + (block_idx + 2) * 8);
    if (!block_ptr) return "NULL";
    uintptr_t entry_addr = block_ptr + 2 * entry_idx;
    uint16_t entry_header = game->rpm<uint16_t>(entry_addr);
    uint32_t name_length = entry_header >> 6;
    if (name_length == 0 || name_length > 255) return "NULL";
    game->ReadRaw(entry_addr + 2, &name, name_length);
    return std::string(name);
}

// Shorten name and add instance number for disambiguation
// fullName = class name from GNames, instanceNum = FName::Number from UObject+0x1C
static std::string ShortenName(const std::string& fullName, int instanceNum) {
    std::string s = fullName;

    // Strip common prefixes
    if (s.find("BP_FirstPersonCharacter_") == 0)
        s = s.substr(24); // remove "BP_FirstPersonCharacter_"
    else if (s.find("BP_") == 0)
        s = s.substr(3);

    // Strip _C suffix (blueprint instance)
    if (s.size() > 2 && s.substr(s.size() - 2) == "_C")
        s = s.substr(0, s.size() - 2);

    // If still has "Character_" in it, try to extract the part before it
    size_t charPos = s.find("_Character");
    if (charPos != std::string::npos)
        s = s.substr(0, charPos);

    // Add instance number to differentiate same-class actors
    if (instanceNum > 0)
        s += " #" + std::to_string(instanceNum);

    return s;
}

// ============================
// WorldToScreen
// ============================
D3DMATRIX Matrix(Vector3 rot, Vector3 origin = Vector3{ 0, 0, 0 }) {
    float radPitch = (rot.x * (float)M_PI / 180.f);
    float radYaw = (rot.y * (float)M_PI / 180.f);
    float radRoll = (rot.z * (float)M_PI / 180.f);
    float SP = sinf(radPitch), CP = cosf(radPitch);
    float SY = sinf(radYaw), CY = cosf(radYaw);
    float SR = sinf(radRoll), CR = cosf(radRoll);
    D3DMATRIX matrix;
    matrix.m[0][0] = CP * CY;  matrix.m[0][1] = CP * SY;  matrix.m[0][2] = SP;    matrix.m[0][3] = 0.f;
    matrix.m[1][0] = SR*SP*CY - CR*SY; matrix.m[1][1] = SR*SP*SY + CR*CY; matrix.m[1][2] = -SR*CP; matrix.m[1][3] = 0.f;
    matrix.m[2][0] = -(CR*SP*CY + SR*SY); matrix.m[2][1] = CY*SR - CR*SP*SY; matrix.m[2][2] = CR*CP; matrix.m[2][3] = 0.f;
    matrix.m[3][0] = origin.x;  matrix.m[3][1] = origin.y;  matrix.m[3][2] = origin.z;  matrix.m[3][3] = 1.f;
    return matrix;
}

// Returns true ONLY if projected position is within screen bounds
bool WorldToScreen(Vector3 WorldLocation, CameraData cam, Vector3& ScreenLocation) {
    D3DMATRIX tempMatrix = Matrix(cam.Rotation);
    Vector3 vAxisX = { tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2] };
    Vector3 vAxisY = { tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2] };
    Vector3 vAxisZ = { tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2] };
    Vector3 vDelta = WorldLocation - cam.Location;
    Vector3 vTransformed = { vDelta.Dot(vAxisY), vDelta.Dot(vAxisZ), vDelta.Dot(vAxisX) };

    if (vTransformed.z < 1.f) {
        ScreenLocation = { 0, 0, 0 };
        return false;
    }

    float fov = cam.FOV;
    float tanHalf = tanf(fov * (float)M_PI / 360.f);
    float screenW = ScreenCenterX * 2.f;
    float screenH = ScreenCenterY * 2.f;

    ScreenLocation.x = ScreenCenterX + vTransformed.x * (ScreenCenterX / tanHalf) / vTransformed.z;
    ScreenLocation.y = ScreenCenterY - vTransformed.y * (ScreenCenterX / tanHalf) / vTransformed.z;

    // Only return true if actually visible on screen (with small margin)
    float margin = 50.f;
    if (ScreenLocation.x < -margin || ScreenLocation.x > screenW + margin ||
        ScreenLocation.y < -margin || ScreenLocation.y > screenH + margin) {
        return false;
    }

    return true;
}

// Get direction angle from camera to world position (for off-screen arrows)
float GetDirectionAngle(Vector3 WorldLocation, CameraData cam) {
    D3DMATRIX tempMatrix = Matrix(cam.Rotation);
    Vector3 vAxisX = { tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2] };
    Vector3 vAxisY = { tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2] };
    Vector3 vDelta = WorldLocation - cam.Location;
    float dx = vDelta.Dot(vAxisY);
    float dy = vDelta.Dot(vAxisX);
    return atan2f(dx, dy);
}

// Draw off-screen indicator arrow on screen edge
void DrawOffscreenArrow(Vector3 worldPos, CameraData cam, D2D1::ColorF color, const char* label, const char* distText) {
    float angle = GetDirectionAngle(worldPos, cam);
    float margin = 40.f;
    float arrowSize = 12.f;

    // Calculate position on screen edge
    float cx = ScreenCenterX;
    float cy = ScreenCenterY;
    float edgeX = cx + sinf(angle) * (cx - margin);
    float edgeY = cy - cosf(angle) * (cy - margin);

    // Clamp to screen
    edgeX = fmaxf(margin, fminf(ScreenCenterX * 2 - margin, edgeX));
    edgeY = fmaxf(margin, fminf(ScreenCenterY * 2 - margin, edgeY));

    // Draw arrow triangle pointing outward
    float ax = sinf(angle) * arrowSize;
    float ay = -cosf(angle) * arrowSize;
    float px = -ay * 0.5f;
    float py = ax * 0.5f;

    g_overlay->draw_line((int)(edgeX + ax), (int)(edgeY + ay), (int)(edgeX - ax + px), (int)(edgeY - ay + py), color);
    g_overlay->draw_line((int)(edgeX + ax), (int)(edgeY + ay), (int)(edgeX - ax - px), (int)(edgeY - ay - py), color);
    g_overlay->draw_line((int)(edgeX - ax + px), (int)(edgeY - ay + py), (int)(edgeX - ax - px), (int)(edgeY - ay - py), color);

    // Label
    g_overlay->draw_text((int)edgeX - 20, (int)edgeY + 15, color, label);
    if (distText)
        g_overlay->draw_text((int)edgeX - 10, (int)edgeY + 28, D2D1::ColorF(255, 255, 0, 255), distText);
}

// ============================
// Read camera
// ============================
void ReadCamera() {
    cameraManager = game->rpm<uintptr_t>(playerController + Offsets::APlayerController_PlayerCameraManager);
    if (!cameraManager) return;
    uintptr_t povAddr = cameraManager + Offsets::APlayerCameraManager_CameraCache + Offsets::FCameraCacheEntry_POV;
    FMinimalViewInfoRead viewInfo;
    game->ReadRaw(povAddr, &viewInfo, sizeof(viewInfo));
    cameraData.Location = { (float)viewInfo.Location[0], (float)viewInfo.Location[1], (float)viewInfo.Location[2] };
    cameraData.Rotation = { (float)viewInfo.Rotation[0], (float)viewInfo.Rotation[1], (float)viewInfo.Rotation[2] };
    cameraData.FOV = viewInfo.FOV;


}

// ============================
// Get bone world position
// ============================
Vector3 GetBonePos(uintptr_t mesh, int boneIndex, uintptr_t rootComp) {
    // Read ComponentToWorld transform from the mesh component
    FTransformRead c2w;
    game->ReadRaw(mesh + Offsets::USceneComponent_ComponentToWorld, &c2w, sizeof(c2w));

    // Read bone TArray<FTransform>
    uintptr_t boneArrayPtr = game->rpm<uintptr_t>(mesh + Offsets::CachedComponentSpaceTransforms);
    int boneCount = game->rpm<int>(mesh + Offsets::CachedComponentSpaceTransforms + 8);

    if (!boneArrayPtr || boneIndex >= boneCount || boneIndex < 0)
        return { 0, 0, 0 };

    // Read bone transform (component space)
    FTransformRead boneTrans;
    game->ReadRaw(boneArrayPtr + boneIndex * Offsets::FTransformSize, &boneTrans, sizeof(boneTrans));

    // Simplified: apply ComponentToWorld translation to bone translation
    // Full quaternion rotation would be more accurate, but for ESP this is good enough
    // bone_world = c2w.Rotation * bone_local + c2w.Translation
    double bx = boneTrans.Translation[0];
    double by = boneTrans.Translation[1];
    double bz = boneTrans.Translation[2];

    // Quaternion rotation: q * v * q^-1
    double qx = c2w.Rotation[0], qy = c2w.Rotation[1], qz = c2w.Rotation[2], qw = c2w.Rotation[3];

    // Rotate vector by quaternion
    double t0 = 2.0 * (qw * bx + qy * bz - qz * by);
    double t1 = 2.0 * (qw * by + qz * bx - qx * bz);
    double t2 = 2.0 * (qw * bz + qx * by - qy * bx);
    double rx = bx + qw * (t0 - 2.0 * (qy * bz - qz * by)) - bx + t0 * qw - t1 * qz + t2 * qy;
    // Use proper formula
    double ix = qw * bx + qy * bz - qz * by;
    double iy = qw * by + qz * bx - qx * bz;
    double iz = qw * bz + qx * by - qy * bx;
    double iw = -qx * bx - qy * by - qz * bz;

    double ox = ix * qw + iw * (-qx) + iy * (-qz) - iz * (-qy);
    double oy = iy * qw + iw * (-qy) + iz * (-qx) - ix * (-qz);
    double oz = iz * qw + iw * (-qz) + ix * (-qy) - iy * (-qx);

    return {
        (float)(ox + c2w.Translation[0]),
        (float)(oy + c2w.Translation[1]),
        (float)(oz + c2w.Translation[2])
    };
}

// Draw bone line between two bones
void DrawBoneLine(uintptr_t mesh, int bone1, int bone2, uintptr_t rootComp, D2D1::ColorF color) {
    Vector3 pos1 = GetBonePos(mesh, bone1, rootComp);
    Vector3 pos2 = GetBonePos(mesh, bone2, rootComp);
    if (pos1.x == 0 && pos1.y == 0 && pos1.z == 0) return;
    if (pos2.x == 0 && pos2.y == 0 && pos2.z == 0) return;
    Vector3 s1, s2;
    if (WorldToScreen(pos1, cameraData, s1) && WorldToScreen(pos2, cameraData, s2)) {
        g_overlay->draw_line((int)s1.x, (int)s1.y, (int)s2.x, (int)s2.y, color);
    }
}

// Draw skeleton
void DrawSkeleton(uintptr_t actor, uintptr_t rootComp, D2D1::ColorF color) {
    uintptr_t mesh = game->rpm<uintptr_t>(actor + Offsets::ACharacter_Mesh);
    if (!mesh) return;

    // Spine
    DrawBoneLine(mesh, Bone::Pelvis, Bone::Chest, rootComp, color);
    DrawBoneLine(mesh, Bone::Chest, Bone::Neck, rootComp, color);
    DrawBoneLine(mesh, Bone::Neck, Bone::Head, rootComp, color);

    // Left arm
    DrawBoneLine(mesh, Bone::Chest, Bone::UpperArmL, rootComp, color);
    DrawBoneLine(mesh, Bone::UpperArmL, Bone::LowerArmL, rootComp, color);
    DrawBoneLine(mesh, Bone::LowerArmL, Bone::HandL, rootComp, color);

    // Right arm
    DrawBoneLine(mesh, Bone::Chest, Bone::UpperArmR, rootComp, color);
    DrawBoneLine(mesh, Bone::UpperArmR, Bone::LowerArmR, rootComp, color);
    DrawBoneLine(mesh, Bone::LowerArmR, Bone::HandR, rootComp, color);

    // Left leg
    DrawBoneLine(mesh, Bone::Pelvis, Bone::ThighL, rootComp, color);
    DrawBoneLine(mesh, Bone::ThighL, Bone::CalfL, rootComp, color);
    DrawBoneLine(mesh, Bone::CalfL, Bone::FootL, rootComp, color);

    // Right leg
    DrawBoneLine(mesh, Bone::Pelvis, Bone::ThighR, rootComp, color);
    DrawBoneLine(mesh, Bone::ThighR, Bone::CalfR, rootComp, color);
    DrawBoneLine(mesh, Bone::CalfR, Bone::FootR, rootComp, color);
}

// ============================
// ESP overlays
// ============================
bool espBox = true;
bool espSkeleton = false;

void ESPLoop() {
    hwnd = FindWindowA("UnrealWindow", NULL);
    hwnd_active = GetForegroundWindow();
    if (Crosshair) {
        g_overlay->draw_line((int)ScreenCenterX - 15, (int)ScreenCenterY - 15, (int)ScreenCenterX + 15, (int)ScreenCenterY + 15, CrosshairColor);
        g_overlay->draw_line((int)ScreenCenterX - 15, (int)ScreenCenterY + 15, (int)ScreenCenterX + 15, (int)ScreenCenterY - 15, CrosshairColor);
    }
}

void RenderMenu() {
    if (showmenu && rendering) {
        // Show role at top
        std::string roleText = "MECCHA CHAMELEON ESP";
        if (pawn) {
            uintptr_t mc = game->rpm<uintptr_t>(pawn + 0x10);
            if (mc) {
                std::string mcn = GetNameById(game->rpm<int>(mc + Offsets::UObject_Name));
                if (mcn.find("Hunter") != std::string::npos) roleText += " [HUNTER]";
                else if (mcn.find("Survivor") != std::string::npos) roleText += " [SURVIVOR]";
                else if (mcn.find("Spectate") != std::string::npos) roleText += " [SPECTATING]";
            }
        }
        g_overlay->draw_text(5, 5, D2D1::ColorF(255, 20, 20, 255), roleText.c_str());

        auto on = D2D1::ColorF(0, 255, 0, 255);
        auto off = D2D1::ColorF(255, 0, 0, 255);

        g_overlay->draw_text(5, 80, Crosshair ? on : off, Crosshair ? "F1 Crosshair : ON" : "F1 Crosshair : OFF");
        g_overlay->draw_text(5, 100, esp ? on : off, esp ? "F2 ESP : ON" : "F2 ESP : OFF");
        g_overlay->draw_text(5, 120, espBox ? on : off, espBox ? "F3 Box : ON" : "F3 Box : OFF");
        g_overlay->draw_text(5, 140, espSkeleton ? on : off, espSkeleton ? "F4 Skeleton : ON" : "F4 Skeleton : OFF");
        g_overlay->draw_text(5, 160, distanceESp ? on : off, distanceESp ? "F5 Distance : ON" : "F5 Distance : OFF");
        g_overlay->draw_text(5, 180, espLine ? on : off, espLine ? "F7 Snapline : ON" : "F7 Snapline : OFF");

        std::string gg = std::to_string(int(distanceMax));
        g_overlay->draw_text(5, 200, D2D1::ColorF(0, 255, 0, 255), "UP/Down MaxDist = ");
        g_overlay->draw_text(140, 200, D2D1::ColorF(0, 0, 255, 255), gg.c_str());
    }
}

// ============================
// Main ESP Logic
// ============================
inline PVOID BaseThread2() {
    if (!esp) return 0;

    debugPlayerCount = 0;
    uWorld = game->rpm<uintptr_t>(process_base + Offsets::GWorld);
    if (!uWorld) return 0;

    gameInstance = game->rpm<uintptr_t>(uWorld + Offsets::UWorld_OwningGameInstance);
    persistentLevel = game->rpm<uintptr_t>(uWorld + Offsets::UWorld_PersistentLevel);
    if (!gameInstance || !persistentLevel) return 0;

    localPlayerPtr = game->rpm<uintptr_t>(gameInstance + Offsets::UGameInstance_LocalPlayers);
    if (!localPlayerPtr) return 0;
    localPlayer = game->rpm<uintptr_t>(localPlayerPtr);
    if (!localPlayer) return 0;
    playerController = game->rpm<uintptr_t>(localPlayer + Offsets::UPlayer_PlayerController);
    if (!playerController) return 0;
    pawn = game->rpm<uintptr_t>(playerController + Offsets::APlayerController_AcknowledgedPawn);

    ReadCamera();
    if (cameraData.FOV <= 0) return 0;

    actorsArray = game->rpm<uintptr_t>(persistentLevel + Offsets::ULevel_Actors);
    actorsCount = game->rpm<int>(persistentLevel + Offsets::ULevel_ActorCount);
    if (!actorsArray || actorsCount <= 0) return 0;
    if (actorsCount > 50000) actorsCount = 50000;



    // Detect local role
    bool localIsHunter = false;
    bool localIsSurvivor = false;
    if (pawn) {
        // Try BP field first (works if pawn is a cLeon character)
        int pawnNameId = game->rpm<int>(pawn + Offsets::UObject_Name);
        std::string pawnName = GetNameById(pawnNameId);
        if (pawnName.find("BP_FirstPersonCharacter") != std::string::npos) {
            localIsHunter = game->rpm<bool>(pawn + Offsets::IsHunter);
            localIsSurvivor = !localIsHunter;
        }
        // If spectating, show everyone (localIsHunter and localIsSurvivor stay false)
    }

    for (int i = 0; i < actorsCount; i++) {
        uintptr_t actor = game->rpm<uintptr_t>(actorsArray + i * 0x8);
        if (!actor || actor == pawn) continue;

        int objectId = game->rpm<int>(actor + Offsets::UObject_Name);
        std::string actorName = GetNameById(objectId);

        if (actorName.find("BP_FirstPersonCharacter") == std::string::npos)
            continue;

        // Read alive status - skip dead players
        bool isAlive = game->rpm<bool>(actor + Offsets::IsLiveSelf);
        if (!isAlive) continue;

        // Read role from BP field (more reliable than UClass name)
        bool isHunter = game->rpm<bool>(actor + Offsets::IsHunter);
        bool isSurvivor = !isHunter;

        // Smart filter: skip same-role players
        if (localIsHunter && isHunter) continue;
        if (localIsSurvivor && isSurvivor) continue;

        debugPlayerCount++;

        uintptr_t rootComp = game->rpm<uintptr_t>(actor + Offsets::AActor_RootComponent);
        if (!rootComp) continue;

        FVector posDouble = game->rpm<FVector>(rootComp + Offsets::USceneComponent_RelativeLocation);
        Vector3 actorPos = { (float)posDouble.x, (float)posDouble.y, (float)posDouble.z };

        float dist = cameraData.Location.DistTo(actorPos);
        float distM = ToMeters(dist);
        if (distM > distanceMax) continue;

        float halfHeight = 90.f;
        Vector3 feetPos = { actorPos.x, actorPos.y, actorPos.z - halfHeight };
        Vector3 headPos = { actorPos.x, actorPos.y, actorPos.z + halfHeight };

        Vector3 screenFeet, screenHead;
        bool feetOnScreen = WorldToScreen(feetPos, cameraData, screenFeet);
        bool headOnScreen = WorldToScreen(headPos, cameraData, screenHead);

        // Don't skip off-screen — we'll draw arrows for them

        // Read health
        double hp = game->rpm<double>(actor + Offsets::Health);
        double maxHp = game->rpm<double>(actor + Offsets::MaxHealthValue);

        // Display name and color based on role
        std::string displayName = isHunter ? "Hunter" : "Survivor";
        std::string distText = std::to_string((int)distM) + "m";

        // Hunter = red, Survivor = green
        auto espColor = isHunter ? D2D1::ColorF(255, 50, 50, 255) : D2D1::ColorF(50, 255, 50, 255);
        auto textColor = isHunter ? D2D1::ColorF(255, 80, 80, 255) : D2D1::ColorF(80, 255, 80, 255);
        auto distColor = D2D1::ColorF(255, 255, 0, 255);
        auto hpColor = D2D1::ColorF(0, 255, 0, 255);

        // Box ESP
        if (espBox && feetOnScreen && headOnScreen) {
            float boxH = screenFeet.y - screenHead.y;
            if (boxH < 5.f) boxH = 5.f;
            float boxW = boxH * 0.4f;
            float boxX = (screenHead.x + screenFeet.x) / 2.f - boxW / 2.f;
            float boxY = screenHead.y;

            int bx = (int)boxX, by = (int)boxY, bw = (int)boxW, bh = (int)boxH;
            int cornerLen = bw / 4;
            if (cornerLen < 3) cornerLen = 3;

            g_overlay->draw_line(bx, by, bx + cornerLen, by, espColor);
            g_overlay->draw_line(bx, by, bx, by + cornerLen, espColor);
            g_overlay->draw_line(bx + bw, by, bx + bw - cornerLen, by, espColor);
            g_overlay->draw_line(bx + bw, by, bx + bw, by + cornerLen, espColor);
            g_overlay->draw_line(bx, by + bh, bx + cornerLen, by + bh, espColor);
            g_overlay->draw_line(bx, by + bh, bx, by + bh - cornerLen, espColor);
            g_overlay->draw_line(bx + bw, by + bh, bx + bw - cornerLen, by + bh, espColor);
            g_overlay->draw_line(bx + bw, by + bh, bx + bw, by + bh - cornerLen, espColor);
        }

        // Skeleton ESP
        if (espSkeleton) {
            DrawSkeleton(actor, rootComp, espColor);
        }

        // Snap line
        if (espLine && feetOnScreen)
            g_overlay->draw_line((int)ScreenCenterX, GetSystemMetrics(SM_CYSCREEN), (int)screenFeet.x, (int)screenFeet.y, espColor);

        // Text info (fixed above box top)
        if (headOnScreen) {
            float boxH = feetOnScreen ? (screenFeet.y - screenHead.y) : 100.f;
            float boxW = boxH * 0.4f;
            int textX = (int)screenHead.x;
            int textY = (int)screenHead.y;

            // Name centered above box
            g_overlay->draw_text(textX - 20, textY - 28, textColor, displayName.c_str());

            // Distance
            if (distanceESp)
                g_overlay->draw_text(textX - 10, textY - 15, distColor, distText.c_str());

            // Health bar below box
            if (feetOnScreen && maxHp > 0) {
                float hpRatio = (float)(hp / maxHp);
                if (hpRatio < 0.f) hpRatio = 0.f;
                if (hpRatio > 1.f) hpRatio = 1.f;
                int barW = (int)boxW;
                if (barW < 20) barW = 20;
                int barX = (int)((screenHead.x + screenFeet.x) / 2.f - barW / 2.f);
                int barY = (int)screenFeet.y + 4;

                // Background
                g_overlay->draw_line(barX, barY, barX + barW, barY, D2D1::ColorF(80, 80, 80, 200));
                g_overlay->draw_line(barX, barY + 1, barX + barW, barY + 1, D2D1::ColorF(80, 80, 80, 200));
                // Fill (green > yellow > red based on HP)
                auto barColor = hpRatio > 0.5f ? D2D1::ColorF(0, 255, 0, 255) :
                               (hpRatio > 0.25f ? D2D1::ColorF(255, 255, 0, 255) : D2D1::ColorF(255, 0, 0, 255));
                int fillW = (int)(barW * hpRatio);
                if (fillW > 0) {
                    g_overlay->draw_line(barX, barY, barX + fillW, barY, barColor);
                    g_overlay->draw_line(barX, barY + 1, barX + fillW, barY + 1, barColor);
                }
            }
        } else if (feetOnScreen) {
            g_overlay->draw_text((int)screenFeet.x - 20, (int)screenFeet.y - 15, textColor, displayName.c_str());
        }

        // Off-screen indicator
        if (!headOnScreen && !feetOnScreen) {
            DrawOffscreenArrow(actorPos, cameraData, espColor, displayName.c_str(), distText.c_str());
        }
    }

    return 0;
}
