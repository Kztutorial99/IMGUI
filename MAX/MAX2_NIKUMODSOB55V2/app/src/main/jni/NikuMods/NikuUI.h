/*=============================================================================
  NikuMods | @ur_nikuu
  A-to-Z clone of the CloneYewalaUi menu (VISUALS / COMBAT / SETTINGS layout).
  Ported 1:1 to run on the NIKUMODSOB55V2 forked imgui (1.87 WIP build).
=============================================================================*/
#pragma once

#ifndef OBFU
#define OBFU(x) x
#endif
#ifdef OBFUSCATE
#undef OBFU
#define OBFU(x) OBFUSCATE(x)
#endif

#include "imgui.h"
#include "imgui_internal.h"
#include "stb_image.h"
#include <cstdio>

//---------------------------------------------------------------------------
// Fonts (MKOP) -- data symbols kept identical to the source menu
//---------------------------------------------------------------------------
#include "Fonts/F47.h"        // Custom3
#include "Fonts/F48.h"        // F48_data / F48_size
#include "Fonts/F19.h"        // F19_data / F19_size
#include "Fonts/F50.h"        // F50_data / F50_size
#include "Fonts/F68.h"        // F68_data / F68_size
#include "Fonts/F86.h"        // F86_data / F86_size
#include "Fonts/Social.h"     // Social_data / New_data / EXTRA_data
#include "Fonts/LoginLogo.h"  // Loginc
#include "Fonts/IMGUI_GOD.h"  // New1_data / poppins_medium / tabs_icons / tahoma_bold / chicon

//---------------------------------------------------------------------------
// Tab / ESP art (MKOP) -- array names identical to the source
//---------------------------------------------------------------------------
#include "Assets/PlayersOn.h"
#include "Assets/PlayersOff.h"
#include "Assets/WorldOn.h"
#include "Assets/WorldOff.h"
#include "Assets/RageBotOn.h"
#include "Assets/RageBotOff.h"
#include "Assets/ScriptsOn.h"
#include "Assets/ScriptsOff.h"
#include "Assets/SettingsOn.h"
#include "Assets/SettingsOff.h"
#include "Assets/Logo.h"
#include "Assets/Aim_off.h"
#include "Assets/Aim_on.h"
#include "Assets/Bt_off.h"
#include "Assets/Bt_on.h"
#include "Assets/Bot_off.h"
#include "Assets/Bot_on.h"
#include "Assets/Enemy_off.h"
#include "Assets/Enemy_on.h"

//---------------------------------------------------------------------------
// Global font pointers (extern-locked by the forked imgui core) + UI fonts
//---------------------------------------------------------------------------
ImFont* poppins_mediumf = nullptr;
ImFont* poppins_medium_low = nullptr;
ImFont* tab_icon = nullptr;
ImFont* tahoma_boldf = nullptr;
ImFont* tahoma_bold2 = nullptr;
ImFont* chicons = nullptr;
ImFont* Social = nullptr;
ImFont* F86 = nullptr;
ImFont* LOGIN = nullptr;
ImFont* Subtab = nullptr;
ImFont* COLOR = nullptr;
ImFont* SR = nullptr;
ImFont* TAB_FONTS = nullptr;
ImFont* F48 = nullptr;
ImFont* F50 = nullptr;
ImFont* F68 = nullptr;
ImFont* EXTRA = nullptr;
ImFont* CLOSE = nullptr;
ImFont* F19 = nullptr;

float menu[4] = { 112 / 255.f, 163 / 255.f, 255 / 255.f, 1.000f };

// Toggle untuk fitur PATCH LIB (libanogs.so)
inline bool LibPatch = false;

//---------------------------------------------------------------------------
// Menu feature switches: every checkbox below points at the REAL hook toggle
// from Struct/Class.h + Struct/Bools.h + Struct/Hooks.h + Struct/main.h (the
// same translation unit as Main.cpp, so NikuUI.h sees them in scope directly).
// The #ifdef NIKU_HARNESS block only feeds the headless test build.
//---------------------------------------------------------------------------
#ifdef NIKU_HARNESS
bool Enable = false;
bool Aimbot = false;
bool SpeedHack = false;
bool norecoil = false;
bool HighFps = false;
float Fov_Aim = 360.0f;
int  AimWhen = 1;
bool Headshot = false;
bool AutoSwap1 = false;
bool SilentAimv222 = false;
bool checkVisible2 = true;
bool ignoreKnocked = true;
bool ignoreEnemyBot = false;
bool Autofire = false;
bool EnemyCount = false;
bool GameTimer = false;
bool FastSwitch = false;
bool SpeedWalking = false;
bool ghostmapcs = false;
bool CoverKill = false;
bool HideDamage = false;
bool GuestResetZ = false;
bool gliderbac = false;
bool infiniteFlyer = false;
struct _cfg {
    struct _esp { bool Box; bool Line; bool Health; } ESP;
} Config;
#endif

//---------------------------------------------------------------------------
// Texture helpers (OpenGL texture upload runs on the device GL context).
// Local harnesses compile with -DNIKU_HARNESS to skip the GL calls.
//---------------------------------------------------------------------------
#ifndef NIKU_HARNESS
#if !defined(__APPLE__)
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#else
#include "imgui_impl_opengl3_loader.h"
#endif
#endif

struct NikuTex
{
    ImTextureID textureId = 0;
    int         w = 0;
    int         h = 0;
};

inline NikuTex NikuCreateTexture(const unsigned char* buf, int len)
{
    NikuTex image;
#ifndef NIKU_HARNESS
    if (!buf || len <= 0)
        return image;
    int imgw = 0, imgh = 0, comp = 0;
    unsigned char* image_data = stbi_load_from_memory(buf, len, &imgw, &imgh, &comp, 4);
    if (image_data == nullptr)
        return image;
    GLuint image_texture = 0;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgw, imgh, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);
    image.textureId = (ImTextureID)image_texture;
    image.w = imgw;
    image.h = imgh;
#else
    (void)buf;
    (void)len;
#endif
    return image;
}

inline NikuTex NikuLogo, NikuAimOff, NikuAimOn, NikuBtOff, NikuBtOn;
inline NikuTex NikuBotOff, NikuBotOn, NikuEnemyOff, NikuEnemyOn;
inline NikuTex NikuPlayers[2], NikuWorld[2], NikuRageBot[2], NikuScripts[2], NikuSettings[2];

inline void NikuInitTextures()
{
    if (NikuLogo.textureId)
        return;
    NikuLogo       = NikuCreateTexture(Logo_data,      sizeof(Logo_data));
    NikuAimOff     = NikuCreateTexture(Aim_off_data,   sizeof(Aim_off_data));
    NikuAimOn      = NikuCreateTexture(Aim_on_data,    sizeof(Aim_on_data));
    NikuBtOff      = NikuCreateTexture(Bt_off_data,    sizeof(Bt_off_data));
    NikuBtOn       = NikuCreateTexture(Bt_on_data,     sizeof(Bt_on_data));
    NikuBotOff     = NikuCreateTexture(Bot_off_data,   sizeof(Bot_off_data));
    NikuBotOn      = NikuCreateTexture(Bot_on_data,    sizeof(Bot_on_data));
    NikuEnemyOff   = NikuCreateTexture(Enemy_off_data, sizeof(Enemy_off_data));
    NikuEnemyOn    = NikuCreateTexture(Enemy_on_data,  sizeof(Enemy_on_data));
    NikuPlayers[0] = NikuCreateTexture(PlayersOff,     sizeof(PlayersOff));
    NikuPlayers[1] = NikuCreateTexture(PlayersOn,      sizeof(PlayersOn));
    NikuWorld[0]   = NikuCreateTexture(WorldOff,       sizeof(WorldOff));
    NikuWorld[1]   = NikuCreateTexture(WorldOn,        sizeof(WorldOn));
    NikuRageBot[0] = NikuCreateTexture(RageBotOff,     sizeof(RageBotOff));
    NikuRageBot[1] = NikuCreateTexture(RageBotOn,      sizeof(RageBotOn));
    NikuScripts[0] = NikuCreateTexture(ScriptsOff,     sizeof(ScriptsOff));
    NikuScripts[1] = NikuCreateTexture(ScriptsOn,      sizeof(ScriptsOn));
    NikuSettings[0] = NikuCreateTexture(SettingsOff,   sizeof(SettingsOff));
    NikuSettings[1] = NikuCreateTexture(SettingsOn,    sizeof(SettingsOn));
}

//---------------------------------------------------------------------------
// Font building -- same load set and sizes as the source menu
//---------------------------------------------------------------------------
void NikuBuildFonts()
{
    ImGuiIO& io = ImGui::GetIO();

    static const ImWchar ranges[] = {
        0x0020, 0x00FF, 0x0400, 0x052F, 0x2DE0, 0x2DFF, 0xA640, 0xA69F,
        0xE000, 0xE226, 0x2010, 0x205E, 0x0600, 0x06FF, 0xFE00, 0xFEFF, 0
    };

    ImFontConfig font_config;
    ImFontConfig CustomFont;
    CustomFont.FontDataOwnedByAtlas = false;
    ImFontConfig cfg;

    io.Fonts->AddFontFromMemoryTTF(const_cast<std::uint8_t*>(Custom3), sizeof(Custom3), 25.f, &CustomFont);
    CLOSE    = io.Fonts->AddFontFromMemoryTTF(Loginc,      sizeof(Loginc),      30.0f, &font_config, ranges);
    F48      = io.Fonts->AddFontFromMemoryTTF((void*)New1_data,  New1_size,     25.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    COLOR    = io.Fonts->AddFontFromMemoryTTF((void*)F19_data,   F19_size,      38.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    F50      = io.Fonts->AddFontFromMemoryTTF((void*)F50_data,   F50_size,      30.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    F68      = io.Fonts->AddFontFromMemoryTTF((void*)F68_data,   F68_size,      54.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    SR       = io.Fonts->AddFontFromMemoryTTF((void*)F48_data,   F48_size,      30.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    LOGIN    = io.Fonts->AddFontFromMemoryTTF(const_cast<std::uint8_t*>(Custom3), sizeof(Custom3), 20.f, &CustomFont);
    Subtab   = io.Fonts->AddFontFromMemoryTTF(const_cast<std::uint8_t*>(Custom3), sizeof(Custom3), 35.f, &CustomFont);
    F86      = io.Fonts->AddFontFromMemoryTTF((void*)F86_data,   F86_size,      30.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    Social   = io.Fonts->AddFontFromMemoryTTF((void*)Social_data, Social_size,  30.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    poppins_mediumf    = io.Fonts->AddFontFromMemoryTTF(poppins_medium, sizeof(poppins_medium), 25.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    poppins_medium_low = io.Fonts->AddFontFromMemoryTTF(poppins_medium, sizeof(poppins_medium), 15.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    tab_icon           = io.Fonts->AddFontFromMemoryTTF(tabs_icons,     sizeof(tabs_icons),     33.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    tahoma_boldf       = io.Fonts->AddFontFromMemoryTTF(tahoma_bold,    sizeof(tahoma_bold),    17.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    tahoma_bold2       = io.Fonts->AddFontFromMemoryTTF(tahoma_bold,    sizeof(tahoma_bold),    28.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    chicons            = io.Fonts->AddFontFromMemoryTTF(chicon,         sizeof(chicon),         20.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    TAB_FONTS = io.Fonts->AddFontFromMemoryTTF((void*)New_data,   New_size,    25.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    EXTRA     = io.Fonts->AddFontFromMemoryTTF((void*)EXTRA_data, EXTRA_size,  25.0f, NULL, io.Fonts->GetGlyphRangesDefault());
}

//---------------------------------------------------------------------------
// Menu render -- exact A-to-Z clone of the source layout (700x650 menu,
// 236px sidebar, 7 tabs, launcher button, FPS + watermark).
//---------------------------------------------------------------------------
static bool  ui_open = false;
static int   iActiveTab = 1;
static int   iActiveSubTab = 1;

void NikuMenu()
{
    ImGui::SetNextWindowSize(ImVec2(120, 120));
    ImGui::Begin("ICON BUTTON", nullptr, ImGuiWindowFlags_NoDecoration); {
        const ImVec2 pos = ImGui::GetWindowPos();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(ImVec2(pos.x + 5, pos.y + 5), ImVec2(pos.x + 115, pos.y + 115), ImColor(10, 10, 10, 255), 100.f, ImDrawFlags_RoundCornersAll);
        ImGui::SetCursorPos(ImVec2(30, 25));
        if (ImGui::OptButton("      ", ImVec2(70, 70), false)) { ui_open = true; }
        draw->AddText(F50, 35.f, ImVec2(30 + pos.x, 35 + pos.y), ImColor(245, 255, 255, 255), OBFU("NM"));
        draw->AddText(LOGIN, 20.f, ImVec2(10 + pos.x, 70 + pos.y), ImColor(0, 165, 243, 255), OBFU("  NIKU"));
        draw->AddText(LOGIN, 20.f, ImVec2(10 + pos.x, 70 + pos.y), ImColor(245, 255, 255, 255), OBFU("            MOD"));
    }
    ImGui::End();   // launcher window closed (matching Begin "ICON BUTTON")

    if (ui_open) {
        ImGui::SetNextWindowSize(ImVec2(700, 650));
            ImGui::MainBegin("Menu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground); {
                const ImVec2 p = ImGui::GetWindowPos();
                ImDrawList* pBackgroundDrawList = ImGui::GetBackgroundDrawList();
                const ImVec2 pos = ImGui::GetWindowPos();
                ImDrawList* draw = ImGui::GetWindowDrawList();

                pBackgroundDrawList->AddRectFilled(ImVec2(0.000f + p.x, 0.000f + p.y), ImVec2(700 + p.x, 650 + p.y), ImColor(0, 0, 0, 255), 5);
                draw->AddRect(ImVec2(pos.x + 0, pos.y + 0), ImVec2(pos.x + 700, pos.y + 650), ImColor(50, 50, 50, 255), 5.f, ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomRight | ImDrawFlags_RoundCornersTopRight, 4.0f);
                draw->AddRectFilled(ImVec2(pos.x + 0, pos.y + 0), ImVec2(pos.x + 236, pos.y + 650), ImColor(23, 23, 23, 255), 0.f, ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersTopLeft);
                draw->AddRectFilled(ImVec2(pos.x + 248, pos.y + 12), ImVec2(pos.x + 606, pos.y + 82), ImColor(23, 23, 23, 255), 5.f, ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomRight | ImDrawFlags_RoundCornersTopRight);
                draw->AddRect(ImVec2(pos.x + 248, pos.y + 12), ImVec2(pos.x + 606, pos.y + 82), ImColor(50, 50, 50, 255), 5.f, ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomRight | ImDrawFlags_RoundCornersTopRight, 2.0f);
                draw->AddRectFilled(ImVec2(pos.x + 618, pos.y + 12), ImVec2(pos.x + 688, pos.y + 82), ImColor(23, 23, 23, 255), 5.f, ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomRight | ImDrawFlags_RoundCornersTopRight);
                draw->AddRect(ImVec2(pos.x + 618, pos.y + 12), ImVec2(pos.x + 688, pos.y + 82), ImColor(50, 50, 50, 255), 5.f, ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomRight | ImDrawFlags_RoundCornersTopRight, 2.0f);
                draw->AddRectFilled(ImVec2(pos.x + 248, pos.y + 94), ImVec2(pos.x + 688, pos.y + 164), ImColor(23, 23, 23, 255), 5.f, ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomRight | ImDrawFlags_RoundCornersTopRight);
                draw->AddRect(ImVec2(pos.x + 248, pos.y + 94), ImVec2(pos.x + 688, pos.y + 164), ImColor(50, 50, 50, 255), 5.f, ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomRight | ImDrawFlags_RoundCornersTopRight, 2.0f);
                draw->AddText(F50, 35.f, ImVec2(258 + p.x, 31.5 + p.y), ImColor(245, 255, 255, 255), OBFU("NIKU MODS"));
                draw->AddText(NULL, 25.f, ImVec2(22 + p.x, 12 - 3.5 + p.y), ImColor(245, 255, 255, 255), OBFU("VISUALS"));
                draw->AddText(NULL, 25.f, ImVec2(22 + p.x, 246 - 3.5 + p.y), ImColor(245, 255, 255, 255), OBFU("COMBAT"));
                draw->AddText(NULL, 25.f, ImVec2(22 + p.x, 470 + p.y), ImColor(245, 255, 255, 255), OBFU("SETTINGS"));
                draw->AddText(CLOSE, 40.f, ImVec2(618 + 20 + p.x, 20 + p.y), ImColor(245, 255, 255, 255 / 2), "i");
                draw->AddRectFilled(ImVec2(pos.x + 248, pos.y + 94), ImVec2(pos.x + 248 + 70, pos.y + 164), ImColor(50, 50, 50, 255), 5.f, ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomRight | ImDrawFlags_RoundCornersTopRight);
                draw->AddRect(ImVec2(pos.x + 13, pos.y + 650 - 13 - 60), ImVec2(pos.x + 236 - 13, pos.y + 650 - 13), ImColor(50, 50, 50, 255), 5.0f, ImDrawFlags_RoundCornersAll, 2.0f);
                static float fps = 0.0f;
                fps = ImGui::GetIO().Framerate;
                char fpsText[32];
                snprintf(fpsText, sizeof(fpsText), "FPS : %.3f", fps);
                draw->AddText(NULL, 35.f, ImVec2(30 + p.x, 680 - 13 - 60 - (35 / 2) + p.y), ImColor(255, 255, 255, 255 / 2), fpsText);

                ImGui::SetCursorPos(ImVec2(618, 12));
                if (ImGui::OptButton(" ", ImVec2(70, 70), false)) { ui_open = false; }
                ImGui::SetCursorPos(ImVec2(12, 42));
                ImGui::BeginGroup(); {
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));
                    if (ImGui::ButtonExes1("A", "ESP MENU", iActiveTab != 1)) { iActiveTab = 1; }
                    if (ImGui::ButtonExes1("G", "BRUTAL", iActiveTab != 2)) { iActiveTab = 2; }
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 42));
                    if (ImGui::ButtonExes1("I", "ESP SETTINGS", iActiveTab != 7)) { iActiveTab = 7; }
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));
                    if (ImGui::ButtonExes1("R", "AIM-BOT", iActiveTab != 3)) { iActiveTab = 3; }
                    if (ImGui::ButtonExes1("O", "SILENT-AIM", iActiveTab != 4)) { iActiveTab = 4; }
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 42));
                    if (ImGui::ButtonExes1("h", "MEMORY", iActiveTab != 5)) { iActiveTab = 5; }
                    if (ImGui::ButtonExes1("a", "SETTINGS", iActiveTab != 6)) { iActiveTab = 6; }
                    ImGui::PopStyleVar(4);
                }
                ImGui::EndGroup();

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(7, 7));

                if (iActiveTab == 1) {
                    draw->AddText(F48, 30.f, ImVec2(pos.x + 268, pos.y + 114), ImColor(245, 255, 255, 255 / 2), "A");
                    draw->AddText(NULL, 25.f, ImVec2(pos.x + 330, pos.y + 114 + 2.5), ImColor(245, 255, 255, 255 / 2), OBFU("BASIC ESP MENU"));
                    ImGui::SetCursorPos(ImVec2(248, 176));
                    ImGui::MenuChild("ESP", ImVec2(440, 462)); {
                        ImGui::Checkbox("Enable Funtion", &Enable);
                        ImGui::Checkbox("Show Line", &Config.ESP.Line);
                        ImGui::Checkbox("Show Box", &Config.ESP.Box);
                        ImGui::Checkbox("Show Health", &Config.ESP.Health);
                        ImGui::SliderFloat("FOV AIM", &Fov_Aim, 360.0f, 360.0f, "%.0f");
                    }
                    ImGui::EndChild();
                }

                if (iActiveTab == 2) {
                    draw->AddText(F48, 30.f, ImVec2(pos.x + 268, pos.y + 114), ImColor(245, 255, 255, 255 / 2), "G");
                    draw->AddText(NULL, 25.f, ImVec2(pos.x + 330, pos.y + 114 + 2.5), ImColor(245, 255, 255, 255 / 2), OBFU("BRUTAL MENU"));
                    ImGui::SetCursorPos(ImVec2(248, 176));
                    ImGui::MenuChild("World", ImVec2(440, 462)); {
                        ImGui::Checkbox("Headshot", &Headshot);
                        ImGui::Checkbox("SpeedHack", &SpeedHack);
                        ImGui::Checkbox("AUTO SWITCH", &AutoSwap1);
                    }
                    ImGui::EndChild();
                }

                if (iActiveTab == 7) {
                    draw->AddText(F48, 30.f, ImVec2(pos.x + 268, pos.y + 114), ImColor(245, 255, 255, 255 / 2), "I");
                    draw->AddText(NULL, 25.f, ImVec2(pos.x + 330, pos.y + 114 + 2.5), ImColor(245, 255, 255, 255 / 2), OBFU("ESP,RANGE AND SETTINGS"));
                    ImGui::SetCursorPos(ImVec2(248, 176));
                    ImGui::MenuChild("ESP SET", ImVec2(440, 462)); {
                        ImGui::Checkbox("Enemy Count", &EnemyCount);
                        ImGui::Checkbox("Game Timer", &GameTimer);
                    }
                    ImGui::EndChild();
                }

                if (iActiveTab == 3) {
                    draw->AddText(F48, 30.f, ImVec2(pos.x + 268, pos.y + 114), ImColor(245, 255, 255, 255 / 2), "R");
                    draw->AddText(NULL, 25.f, ImVec2(pos.x + 330, pos.y + 114 + 2.5), ImColor(245, 255, 255, 255 / 2), OBFU("AIMBOT,TRIGGERS AND SETTINGS"));
                    ImGui::SetCursorPos(ImVec2(248, 176));
                    ImGui::MenuChild("AimBot", ImVec2(440, 462)); {
                        ImGui::Checkbox("Enable AimBot", &Aimbot);
                        static bool aimAlways = false, aimFire = false, aimScope = false;
                        aimAlways = (AimWhen == 0);
                        aimFire   = (AimWhen == 1);
                        aimScope  = (AimWhen == 2);
                        if (ImGui::Checkbox("AimWhen", &aimAlways)) { AimWhen = (aimAlways ? 0 : 1); }
                        if (ImGui::Checkbox("AimFire", &aimFire))   { AimWhen = (aimFire   ? 1 : 0); }
                        if (ImGui::Checkbox("AimScope", &aimScope)) { AimWhen = (aimScope  ? 2 : 0); }
                    }
                    ImGui::EndChild();
                }

                if (iActiveTab == 4) {
                    draw->AddText(F48, 30.f, ImVec2(pos.x + 268, pos.y + 114), ImColor(245, 255, 255, 255 / 2), "O");
                    draw->AddText(NULL, 25.f, ImVec2(pos.x + 330, pos.y + 114 + 2.5), ImColor(245, 255, 255, 255 / 2), OBFU("BULLET-TRACK AND SETTINGS"));
                    ImGui::SetCursorPos(ImVec2(248, 176));
                    ImGui::MenuChild("Silent-Aim", ImVec2(440, 462)); {
                        ImGui::Checkbox("Enable SilentAim", &SilentAimv222);
                        ImGui::Checkbox("Visibility Check", &checkVisible2);
                        ImGui::Checkbox("Ignore Knock", &ignoreKnocked);
                        ImGui::Checkbox("Ignore Bot", &ignoreEnemyBot);
                        ImGui::Checkbox("Auto Fire", &Autofire);
                    }
                    ImGui::EndChild();
                }

                if (iActiveTab == 5) {
                    draw->AddText(F48, 30.f, ImVec2(pos.x + 268, pos.y + 114), ImColor(245, 255, 255, 255 / 2), "h");
                    draw->AddText(NULL, 25.f, ImVec2(pos.x + 330, pos.y + 114 + 2.5), ImColor(245, 255, 255, 255 / 2), OBFU("SUPER RISKY FEATURES"));
                    ImGui::SetCursorPos(ImVec2(248, 176));
                    ImGui::MenuChild("Memory", ImVec2(440, 462)); {
                        ImGui::Checkbox("PATCH LIB (libanogs.so)", &LibPatch);
                        ImGui::Checkbox("No Recoil", &norecoil);
                        ImGui::Checkbox("Ghost Map", &ghostmapcs);
                        ImGui::Checkbox("Cover Kill", &CoverKill);
                        ImGui::Checkbox("Fast Switch", &FastSwitch);
                        ImGui::Checkbox("Hide Damage", &HideDamage);
                        ImGui::Checkbox("Reset Guest", &GuestResetZ);
                        ImGui::Checkbox("Glider Inst", &gliderbac);
                        ImGui::Checkbox("90 FPS", &HighFps);
                        ImGui::Checkbox("Infinite Fly", &infiniteFlyer);
                        ImGui::Checkbox("Speed Walk", &SpeedWalking);
                    }
                    ImGui::EndChild();
                }

                if (iActiveTab == 6) {
                    draw->AddText(F48, 30.f, ImVec2(pos.x + 268, pos.y + 114), ImColor(245, 255, 255, 255 / 2), "a");
                    draw->AddText(NULL, 25.f, ImVec2(pos.x + 330, pos.y + 114 + 2.5), ImColor(245, 255, 255, 255 / 2), OBFU("MENU SETTINGS AND INFO"));
                    ImGui::SetCursorPos(ImVec2(248, 176));
                    ImGui::MenuChild("Extra", ImVec2(440, 462)); {
                        ImGui::ColorEdit3("Menu Color", (float*)&menu, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs);
                    }
                    ImGui::EndChild();
                }

                ImGui::PopStyleVar();
            } // end MainBegin code block (still inside the menu window)
            ImGui::End();   // menu window closed (matching MainBegin "Menu")
        } // end if(ui_open)
}
