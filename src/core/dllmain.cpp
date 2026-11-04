#include <windows.h>
#include <cstdio>
#include <cstring>
#include "overlay/dx9_hook.h"

namespace {
    HANDLE g_instanceMutex = nullptr;

    FILE* AbrirLog() {
        char path[MAX_PATH];
        const DWORD n = GetTempPathA(MAX_PATH, path);
        if (!n || n >= MAX_PATH) return nullptr;
        strcat_s(path, "l4d2_thirdperson.log");
        return fopen(path, "a");
    }
    void Log(const char* message) {
        FILE* file = AbrirLog();
        if (file) { fprintf(file, "%s\n", message); fclose(file); }
    }
    HWND BuscarVentana() {
        HWND found = nullptr;
        EnumWindows([](HWND window, LPARAM data)->BOOL {
            HWND* result = reinterpret_cast<HWND*>(data);
            DWORD pid = 0;
            GetWindowThreadProcessId(window, &pid);
            if (pid != GetCurrentProcessId() || !IsWindowVisible(window)) return TRUE;
            char title[256] = {};
            GetWindowTextA(window, title, sizeof(title));
            if (strstr(title, "Left 4 Dead")) { *result = window; return FALSE; }
            if (!*result) *result = window;
            return TRUE;
        }, reinterpret_cast<LPARAM>(&found));
        return found;
    }
    void Setup() {
        char name[96];
        sprintf_s(name, "Local\\l4d2_thirdperson_%lu", GetCurrentProcessId());
        HANDLE mutex = CreateMutexA(nullptr, FALSE, name);
        const DWORD error = GetLastError();
        if (!mutex) { Log("[setup] no se pudo crear mutex"); return; }
        if (error == ERROR_ALREADY_EXISTS) {
            Log("[setup] ya hay una instancia activa");
            CloseHandle(mutex);
            return;
        }
        g_instanceMutex = mutex;
        for (int i = 0; i < 300 && !GetModuleHandleA("client.dll"); ++i) Sleep(100);
        for (int i = 0; i < 300 && !GetModuleHandleA("d3d9.dll"); ++i) Sleep(100);
        HWND window = BuscarVentana();
        if (!dx9h::Instalar(window)) Log("[dx9] fallo al instalar el overlay");
        else Log("[dx9] overlay de tercera persona iniciado");
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        HMODULE resident = nullptr;
        if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCSTR>(module), &resident)) return FALSE;
        DisableThreadLibraryCalls(module);
        HANDLE thread = CreateThread(nullptr, 0, [](LPVOID)->DWORD { Setup(); return 0; }, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
