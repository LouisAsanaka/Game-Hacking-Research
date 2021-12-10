// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>
#include <iostream>
#include <TlHelp32.h>
#include "mem.h"
#include "process.h"
#include "hook.h"
#include "glDraw.h"
#include "glText.h"
#include "esp.h"
#include "aimbot.h"

// Get module base address
uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"ac_client.exe");

bool bHealth = false, bAmmo = false, bRecoil = false, bTrigger = false, bAimbot = false;

typedef Entity* (__cdecl* tGetCrosshairEntity)();
tGetCrosshairEntity GetCrosshairEntity = (tGetCrosshairEntity) (moduleBase + 0x607c0);

typedef BOOL(__stdcall* twglSwapBuffers) (HDC hdc);
twglSwapBuffers wglSwapBuffersGateway;

GL::Font glFont;
const int FONT_HEIGHT = 15;
const int FONT_WIDTH = 8;

ESP esp;
Aimbot aimbot;

void Draw() {
    HDC currentHDC = wglGetCurrentDC();
    if (!glFont.bBuilt || currentHDC != glFont.hdc) {
        glFont.Build(FONT_HEIGHT);
    }
    GL::SetupOrtho();
   
    esp.Draw(glFont);

    GL::RestoreGL();
}

BOOL __stdcall hookedwglSwapBuffers(HDC hdc) {
    // Key input
    if (GetAsyncKeyState(VK_F1) & 1) {
        bHealth = !bHealth;
        std::cout << "Toggled health hack" << std::endl;
    }
    if (GetAsyncKeyState(VK_F2) & 1) {
        bAmmo = !bAmmo;
        std::cout << "Toggled ammo hack" << std::endl;
    }
    if (GetAsyncKeyState(VK_F3) & 1) {
        bRecoil = !bRecoil;
        if (bRecoil) {
            mem::Nop((BYTE*)(moduleBase + 0x63786), 10);
        } else {
            mem::Patch((BYTE*)(moduleBase + 0x63786), (BYTE*)"\x50\x8d\x4c\x24\x1c\x51\x8b\xce\xff\xd2", 10);
        }
        std::cout << "Toggled recoil hack" << std::endl;
    }
    if (GetAsyncKeyState(VK_F4) & 1) {
        bTrigger = !bTrigger;
        std::cout << "Toggled trigger hack" << std::endl;
    }
    if (GetAsyncKeyState(VK_F5) & 1) {
        bAimbot = !bAimbot;
        std::cout << "Toggled aimbot hack" << std::endl;
    }
    if (GetAsyncKeyState(VK_F12) & 1) {
        // TODO: Unload DLL
    }
    // Continuous write
    Entity* localPlayerPtr = *(Entity**)(moduleBase + 0x10f4f4);
    if (localPlayerPtr != nullptr) {
        if (bHealth) {
            localPlayerPtr->Health = 1337;
        }
        if (bAmmo) {
            localPlayerPtr->CurrentWeapon->AmmoMag->Ammo = 1337;
        }
        if (bTrigger) {
            Entity* target = GetCrosshairEntity();
            if (target != nullptr && target->Team != localPlayerPtr->Team) {
                std::cout << "Found target: " << target->Name << std::endl;
                localPlayerPtr->Attacking = 0x1;
            } else {
                localPlayerPtr->Attacking = 0x0;
            }
        }
        if (bAimbot) {
            aimbot.Update();
        } else {
            localPlayerPtr->Attacking = false;
        }
    }

    Draw();

    return wglSwapBuffersGateway(hdc);
}

DWORD WINAPI HackThread(HMODULE hModule) {
    // Create console
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    //std::cout << "Hello World" << std::endl;

    // Hook wglSwapBuffers
    Hook SwapBuffersHook{ "wglSwapBuffers", "opengl32.dll", (BYTE*)hookedwglSwapBuffers, (BYTE*)&wglSwapBuffersGateway, 5 };
    SwapBuffersHook.Enable();

    Sleep(20000);

    SwapBuffersHook.Disable();
    Sleep(1000);

    // Cleanup & eject
    fclose(f);
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
        CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)HackThread, hModule, 0, nullptr));
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

