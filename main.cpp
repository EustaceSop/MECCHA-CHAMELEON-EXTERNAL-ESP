#pragma once
#include "global.h"
#include "ESP.h"
#include <thread>

void Cleanup() {
    rendering = false;
    shouldExit = true;
    Sleep(100);
    if (g_overlay) {
        g_overlay->clear_screen();
        g_overlay->d2d_shutdown();
    }
    if (game && game->hProcess) {
        CloseHandle(game->hProcess);
        game->hProcess = NULL;
    }
    if (game) { delete game; game = nullptr; }
    ExitProcess(0);
}

void Update() {
    while (!shouldExit) {
        if (GetAsyncKeyState(VK_END) & 1) { Cleanup(); return; }
        if (rendering) {
            if (GetAsyncKeyState(VK_F1) & 1) if (showmenu) Crosshair = !Crosshair;
            if (GetAsyncKeyState(VK_F2) & 1) if (showmenu) esp = !esp;
            if (GetAsyncKeyState(VK_F3) & 1) if (showmenu) espBox = !espBox;
            if (GetAsyncKeyState(VK_F5) & 1) if (showmenu) distanceESp = !distanceESp;
            if (GetAsyncKeyState(VK_F7) & 1) if (showmenu) espLine = !espLine;
            if (GetAsyncKeyState(VK_UP) & 1) { if (showmenu && distanceMax < 3000) distanceMax += 10.f; }
            if (GetAsyncKeyState(VK_DOWN) & 1) { if (showmenu && distanceMax > 10) distanceMax -= 10.f; }
        }
        if (GetAsyncKeyState(VK_INSERT) & 1) showmenu = !showmenu;
        Sleep(10);
    }
}

static void render(FOverlay* overlay) {
    while (!shouldExit) {
        overlay->begin_scene();
        overlay->clear_scene();
        BaseThread2();
        RenderMenu();
        ESPLoop();
        overlay->end_scene();
    }
}

static void _init(FOverlay* overlay) {
    if (!overlay->window_init()) { Sleep(5000); return; }
    if (!overlay->init_d2d()) return;
    std::thread r(render, overlay);
    std::thread up(Update);
    r.join();
    up.join();
    overlay->d2d_shutdown();
}

int main() {
    printf("MECCHA CHAMELEON ESP | GAME VERSION 1.3.1\n");
    printf("Waiting for game...\n");

    const char* processName = "PenguinHotel-Win64-Shipping.exe";
    while (!game->Attach(processName)) Sleep(1000);
    printf("Attached (PID: %d)\n", game->processId);

    process_base = game->GetModuleBase();
    if (!process_base) { printf("Failed to get base! Run as admin.\n"); Sleep(5000); return 1; }

    uintptr_t testWorld = game->rpm<uintptr_t>(process_base + Offsets::GWorld);
    if (!testWorld) {
        printf("Waiting for game to load...\n");
        while (!testWorld) { testWorld = game->rpm<uintptr_t>(process_base + Offsets::GWorld); Sleep(1000); }
    }

    ScreenCenterX = (float)(GetSystemMetrics(SM_CXSCREEN) / 2);
    ScreenCenterY = (float)(GetSystemMetrics(SM_CYSCREEN) / 2);

    printf("Ready! Controls: INSERT=Menu, F2=ESP, END=Exit\n");

    g_overlay = new FOverlay();
    _init(g_overlay);
    return 0;
}
