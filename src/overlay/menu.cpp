#include "menu.h"
#include "thirdperson/thirdperson.h"
#include "dx9_hook.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "notifications.h"
#include "core/config_file.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "inter_font.h"
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cmath>

namespace menu {
    static bool g_visible = false;
    static bool g_menuJuego = false;
    static bool g_cursorBloqueado = false;
    static POINT g_cursorAntes = {};
    static bool g_render = false;
    static ULONGLONG g_animInicio = 0;
    static HWND g_hwnd = nullptr;
    static ImVec2 g_panelPos(-1.0f, -1.0f);

    static const ImVec4 C_BG(0.067f, 0.071f, 0.078f, 1.0f);
    static const ImVec4 C_PANEL(0.105f, 0.113f, 0.125f, 1.0f);
    static const ImVec4 C_ACCENT(0.55f, 0.86f, 0.72f, 1.0f);
    static const ImVec4 C_ACCENT_H(0.66f, 0.94f, 0.81f, 1.0f);
    static const ImVec4 C_BORDER(0.19f, 0.20f, 0.22f, 1.0f);
    static const ImVec4 C_DIM(0.58f, 0.60f, 0.63f, 1.0f);
    static const ImVec4 C_TXT(0.93f, 0.94f, 0.95f, 1.0f);
    static const ImVec4 C_ERROR(0.94f, 0.54f, 0.51f, 1.0f);

    static bool g_dirty = false;
    static bool g_saved = false;
    static bool g_saveError = false;
    static int g_tpHotkey = 0;
    static bool g_tpCapturando = false;
    static bool g_tpTeclaAntes = true;
    static bool g_tpCapturaAbajo = false;
    static ULONGLONG g_tpCapturaInicio = 0;
    static bool g_teclaAntes = true;
    static notices::Queue g_notices;

    static void Dirty() { g_dirty = true; g_saveError = false; }
    static void NotificarCambio(const char* name, bool enabled) {
        g_notices.Show(name, enabled, GetTickCount64());
    }
    static bool TeclaAsignable(int vk) {
        return vk >= 0x08 && vk <= 0xFE && vk != VK_ESCAPE &&
            vk != VK_CONTROL && vk != VK_LCONTROL && vk != VK_RCONTROL &&
            vk != VK_MENU && vk != VK_LMENU && vk != VK_RMENU &&
            vk != VK_SHIFT && vk != VK_LSHIFT && vk != VK_RSHIFT &&
            vk != VK_LWIN && vk != VK_RWIN;
    }
    static std::wstring RutaConfig() {
        return configfile::BesideModule(reinterpret_cast<const void*>(&RutaConfig));
    }
    static float NumeroConfig(const char* text, float minimum, float maximum, float fallback) {
        char* end = nullptr;
        const double value = strtod(text, &end);
        if (end == text || *end || !std::isfinite(value)) return fallback;
        return float(value < minimum ? minimum : value > maximum ? maximum : value);
    }
    static void CargarConfig() {
        g_notices.Clear();
        tp::g_activado = false;
        tp::g_distancia = tp::DISTANCIA_DEFAULT;
        g_tpHotkey = 0;
        const auto path = RutaConfig();
        FILE* f = path.empty() ? nullptr : _wfopen(path.c_str(), L"r");
        if (!f) return;
        char linea[256];
        while (fgets(linea, sizeof(linea), f)) {
            char clave[64] = {}, valor[128] = {};
            if (sscanf_s(linea, "%63s = %127s", clave, (unsigned)_countof(clave),
                         valor, (unsigned)_countof(valor)) == 2) {
                if (!strcmp(clave, "tpActivado")) tp::g_activado = atoi(valor) == 1;
                else if (!strcmp(clave, "tpDistancia"))
                    tp::g_distancia = NumeroConfig(valor, tp::DISTANCIA_MIN, tp::DISTANCIA_MAX, tp::DISTANCIA_DEFAULT);
                else if (!strcmp(clave, "tpTecla")) {
                    const int key = atoi(valor);
                    if (TeclaAsignable(key) && key != VK_INSERT) g_tpHotkey = key;
                }
            }
        }
        fclose(f);
    }
    static void GuardarConfig() {
        g_saved = false;
        configfile::AtomicFile pending;
        FILE* f = pending.Open(RutaConfig());
        if (!f) { g_saveError = true; return; }
        const bool ok = fprintf(f, "tpActivado = %d\ntpDistancia = %g\ntpTecla = %d\n",
            tp::g_activado ? 1 : 0, tp::LimitarDistancia(tp::g_distancia), g_tpHotkey) > 0;
        const bool committed = pending.Commit(ok);
        g_saveError = !committed;
        if (committed) { g_dirty = false; g_saved = true; }
    }

    void Inicializar(HWND hwnd) {
        g_hwnd = hwnd;
        ImGuiIO& io = ImGui::GetIO();
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        io.Fonts->AddFontFromMemoryTTF((void*)inter_font, sizeof(inter_font), 14.0f,
            &fontConfig, io.Fonts->GetGlyphRangesDefault());
        CargarConfig();
    }
    bool estaVisible() { return g_visible; }
    bool estaEnMenuJuego() { return g_visible && g_menuJuego; }
    bool tieneFoco() {
        HWND foreground = GetForegroundWindow();
        return g_hwnd && foreground &&
            GetAncestor(foreground, GA_ROOT) == GetAncestor(g_hwnd, GA_ROOT);
    }
    static void LiberarBloqueo() {
        if (!g_cursorBloqueado) return;
        ClipCursor(nullptr);
        SetCursorPos(g_cursorAntes.x, g_cursorAntes.y);
        g_cursorBloqueado = false;
    }
    void PerderFoco() {
        g_tpCapturando = false; g_tpCapturaInicio = 0; g_tpCapturaAbajo = false;
        g_tpTeclaAntes = true;
        if (g_cursorBloqueado) { ClipCursor(nullptr); g_cursorBloqueado = false; }
        g_visible = false; g_render = false; g_menuJuego = false;
        g_teclaAntes = true;
        CURSORINFO ci = { sizeof(ci) };
        if (GetCursorInfo(&ci) && !(ci.flags & CURSOR_SHOWING)) ShowCursor(TRUE);
        if (ImGui::GetCurrentContext()) ImGui::GetIO().MouseDrawCursor = false;
    }
    bool CancelarCaptura() {
        const bool active = g_tpCapturando;
        g_tpCapturando = false; g_tpCapturaInicio = 0; g_tpCapturaAbajo = false;
        if (active) g_tpTeclaAntes = true;
        return active;
    }
    void Cerrar() {
        if (!g_visible) return;
        CancelarCaptura(); g_visible = false; g_menuJuego = false;
        g_animInicio = GetTickCount64(); g_render = true;
        LiberarBloqueo(); ClipCursor(nullptr); g_cursorBloqueado = false;
    }

    static void ApplyModernStyle() {
        ImGuiStyle& s = ImGui::GetStyle();
        ImVec4* c = s.Colors;
        s.WindowRounding = 8.0f; s.ChildRounding = 4.0f; s.FrameRounding = 4.0f;
        s.PopupRounding = 4.0f; s.ScrollbarRounding = 3.0f; s.GrabRounding = 3.0f;
        s.WindowBorderSize = 1.0f; s.FrameBorderSize = 0.0f; s.PopupBorderSize = 1.0f;
        s.WindowPadding = ImVec2(18, 14); s.FramePadding = ImVec2(10, 6);
        s.ItemSpacing = ImVec2(10, 8); s.ItemInnerSpacing = ImVec2(8, 6);
        s.ScrollbarSize = 5.0f; s.GrabMinSize = 10.0f;
        c[ImGuiCol_Text] = C_TXT; c[ImGuiCol_TextDisabled] = C_DIM;
        c[ImGuiCol_WindowBg] = C_BG; c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);
        c[ImGuiCol_Border] = C_BORDER; c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_FrameBg] = C_PANEL; c[ImGuiCol_FrameBgHovered] = ImVec4(0.10f, 0.10f, 0.10f, 1);
        c[ImGuiCol_FrameBgActive] = ImVec4(0.14f, 0.14f, 0.14f, 1);
        c[ImGuiCol_TitleBg] = C_BG; c[ImGuiCol_TitleBgActive] = C_BG; c[ImGuiCol_TitleBgCollapsed] = C_BG;
        c[ImGuiCol_Button] = C_PANEL; c[ImGuiCol_ButtonHovered] = ImVec4(0.13f, 0.13f, 0.13f, 1);
        c[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.16f, 0.16f, 1);
        c[ImGuiCol_SliderGrab] = C_ACCENT; c[ImGuiCol_SliderGrabActive] = C_ACCENT_H;
        c[ImGuiCol_CheckMark] = C_ACCENT; c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_ScrollbarGrab] = ImVec4(0.18f, 0.18f, 0.18f, 1);
        c[ImGuiCol_ScrollbarGrabHovered] = C_DIM; c[ImGuiCol_ScrollbarGrabActive] = C_ACCENT;
        c[ImGuiCol_Separator] = C_BORDER; c[ImGuiCol_Header] = C_PANEL;
        c[ImGuiCol_TextSelectedBg] = ImVec4(0.14f, 0.14f, 0.14f, 1);
    }
    static const char* NombreTecla(int vk) {
        static char buf[64];
        if (vk <= 0) return "Sin tecla";
        if (vk == VK_INSERT) return "Insert";
        const UINT sc = MapVirtualKey((UINT)vk, MAPVK_VK_TO_VSC_EX);
        const LONG lp = (LONG)(((sc & 0xFF) << 16) | ((sc & 0xFF00) ? (1 << 24) : 0));
        if (GetKeyNameTextA(lp, buf, sizeof(buf)) > 0) return buf;
        return "?";
    }

    #include "native_pages.inl"

    void Dibujar() {
        static bool estilo = false;
        if (!estilo) { ApplyModernStyle(); estilo = true; }
        if (!tieneFoco()) {
            if (g_visible || g_cursorBloqueado || g_tpCapturando) PerderFoco();
            g_teclaAntes = true; g_tpTeclaAntes = true;
            return;
        }

        const bool capturing = g_tpCapturando;
        if (capturing && (GetAsyncKeyState(VK_ESCAPE) & 0x8000)) CancelarCaptura();
        const bool menuKeyDown = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
        if (menuKeyDown && !g_teclaAntes && !capturing && !ImGui::GetIO().WantTextInput) {
            if (g_visible) Cerrar();
            else {
                CURSORINFO ci = { sizeof(ci) };
                g_menuJuego = GetCursorInfo(&ci) && (ci.flags & CURSOR_SHOWING);
                g_visible = true; g_animInicio = GetTickCount64(); g_render = true;
            }
        }
        g_teclaAntes = menuKeyDown;

        const bool tpKeyDown = g_tpHotkey != 0 && (GetAsyncKeyState(g_tpHotkey) & 0x8000) != 0;
        if (tpKeyDown && !g_tpTeclaAntes && !capturing && !ImGui::GetIO().WantTextInput) {
            tp::g_activado = !tp::g_activado;
            NotificarCambio("Tercera persona", tp::g_activado);
            Dirty();
        }
        g_tpTeclaAntes = tpKeyDown;

        if (g_tpCapturando) {
            if (!g_tpCapturaInicio) g_tpCapturaInicio = GetTickCount64();
            int key = 0;
            for (int vk = 0x08; vk <= 0xFE; ++vk) {
                if (vk == VK_INSERT || !TeclaAsignable(vk)) continue;
                if (GetAsyncKeyState(vk) & 0x8000) { key = vk; break; }
            }
            if (GetTickCount64() - g_tpCapturaInicio > 150 && key && !g_tpCapturaAbajo) {
                g_tpHotkey = key; g_tpCapturando = false; g_tpCapturaInicio = 0;
                g_tpTeclaAntes = true; GuardarConfig();
            }
            g_tpCapturaAbajo = key != 0;
        } else { g_tpCapturaInicio = 0; g_tpCapturaAbajo = false; }

        const float duration = 200.0f;
        float t = float(GetTickCount64() - g_animInicio) / duration;
        if (t > 1.0f) t = 1.0f;
        const float eased = t * t * (3.0f - 2.0f * t);
        const float alpha = g_visible ? eased : 1.0f - eased;
        const float slide = g_visible ? (1.0f - eased) * 10.0f : -eased * 10.0f;
        if (!g_visible && t >= 1.0f) g_render = false;
        if (!g_render) {
            LiberarBloqueo();
            if (ImGui::GetCurrentContext()) ImGui::GetIO().MouseDrawCursor = false;
            g_notices.Draw(GetTickCount64());
            return;
        }

        if (g_visible) {
            if (g_menuJuego) {
                CURSORINFO ci = { sizeof(ci) };
                if (!GetCursorInfo(&ci) || !(ci.flags & CURSOR_SHOWING)) g_menuJuego = false;
            }
            if (g_menuJuego) {
                LiberarBloqueo();
                if (ImGui::GetCurrentContext()) ImGui::GetIO().MouseDrawCursor = false;
            } else {
                ReleaseCapture();
                if (!g_cursorBloqueado) { GetCursorPos(&g_cursorAntes); g_cursorBloqueado = true; }
                SetCursorPos(g_cursorAntes.x, g_cursorAntes.y);
                RECT limit = { g_cursorAntes.x, g_cursorAntes.y, g_cursorAntes.x + 1, g_cursorAntes.y + 1 };
                ClipCursor(&limit);
                CURSORINFO ci = { sizeof(ci) };
                if (GetCursorInfo(&ci) && (ci.flags & CURSOR_SHOWING)) ShowCursor(FALSE);
                if (ImGui::GetCurrentContext()) ImGui::GetIO().MouseDrawCursor = true;
            }
        } else {
            LiberarBloqueo();
            if (ImGui::GetCurrentContext()) ImGui::GetIO().MouseDrawCursor = false;
            CURSORINFO ci = { sizeof(ci) };
            if (GetCursorInfo(&ci) && !(ci.flags & CURSOR_SHOWING)) ShowCursor(TRUE);
        }
        DrawNative(alpha, slide);
        g_notices.Draw(GetTickCount64());
    }
}
