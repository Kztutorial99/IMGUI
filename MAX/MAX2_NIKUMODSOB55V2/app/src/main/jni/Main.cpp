#include <android/log.h>
#include <unistd.h>
#include <thread>
#include <limits>
#include <xdl.h>
#include <KittyUtils.h>
#include <KittyMemory.h>
#include <Il2Cpp.h>
#include <SubstrateHook.h>
#include <CydiaSubstrate.h>
#include "Struct/Gui.hpp"
#include <Struct/main.h>
#include "KittyMemory/MemoryPatch.h"
#include "ModConfig.h"
#include "ZaruPatch.h"
#include "CurlAuth.h"
#include "NikuMods/NikuUI.h"   // NikuMods | @ur_nikuu  (cloned A-to-Z menu)
#define ZPATCH(offset, hex) do{ \
    uintptr_t _addr = (uintptr_t)(offset); \
    if(_addr && _addr > 0x1000) ZaruPatch(_addr, hex).Modify(); \
}while(0)

#define PATCH_RET(addr) MemoryPatch::createWithHex((uintptr_t)(addr), OBFUSCATE("C0 03 5F D6")).Modify()
#define PATCH_RET_TRUE(addr) MemoryPatch::createWithHex((uintptr_t)(addr), OBFUSCATE("20 00 80 D2 C0 03 5F D6")).Modify()
#define PATCH_RET_FALSE(addr) MemoryPatch::createWithHex((uintptr_t)(addr), OBFUSCATE("00 00 80 D2 C0 03 5F D6")).Modify()
#define FAL "00 00 80 52 C0 03 5F D6"
#define TRU "20 00 80 52 C0 03 5F D6"
#define MULL "00 00 80 D2 C0 03 5F D6"
#define RET "C0 03 5F D6"


static int currentTab = 0;
bool ImGuiOK = true;
bool clearMousePos = true;
bool initImGui = false;

uintptr_t il2cpp_base = 0;
void *getRealAddr(ulong offset) {
    return reinterpret_cast<void*>(il2cpp_base + offset);
}

namespace Settings {
    static int Tab = 1;
}
#define Application_OpenURL(url) ((void(*)(Il2CppString*))Il2CppGetMethodOffset(OBFUSCATE("UnityEngine.CoreModule.dll"), OBFUSCATE("UnityEngine"), OBFUSCATE("Application"), OBFUSCATE("OpenURL"), 1))(url)

static CurlAuth auth;
static char inputKeyBuffer[256] = {0};
static std::string authMessage = "";
static bool authMessageError = false;
static bool gettingKey = false;
static bool showAuthWindow = true;
static bool authSuccess = false;

void SetJavaVM(JavaVM* vm) {
    jvm = vm;
}

std::string getClipboard() {
    if (!jvm) return "";
    
    std::string result;
    JNIEnv *env;
    
    jvm->AttachCurrentThread(&env, NULL);
    
    auto looperClass = env->FindClass("android/os/Looper");
    auto prepareMethod = env->GetStaticMethodID(looperClass, "prepare", "()V");
    env->CallStaticVoidMethod(looperClass, prepareMethod);
    
    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    jfieldID sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, "sCurrentActivityThread", "Landroid/app/ActivityThread;");
    jobject sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass, sCurrentActivityThreadField);
    
    jfieldID mInitialApplicationField = env->GetFieldID(activityThreadClass, "mInitialApplication", "Landroid/app/Application;");
    jobject mInitialApplication = env->GetObjectField(sCurrentActivityThread, mInitialApplicationField);
    
    auto contextClass = env->FindClass("android/content/Context");
    auto getSystemServiceMethod = env->GetMethodID(contextClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    
    auto str = env->NewStringUTF("clipboard");
    auto clipboardManager = env->CallObjectMethod(mInitialApplication, getSystemServiceMethod, str);
    env->DeleteLocalRef(str);
    
    jclass ClipboardManagerClass = env->FindClass("android/content/ClipboardManager");
    auto getText = env->GetMethodID(ClipboardManagerClass, "getText", "()Ljava/lang/CharSequence;");
    
    jclass CharSequenceClass = env->FindClass("java/lang/CharSequence");
    auto toStringMethod = env->GetMethodID(CharSequenceClass, "toString", "()Ljava/lang/String;");
    
    auto text = env->CallObjectMethod(clipboardManager, getText);
    if (text) {
        str = (jstring) env->CallObjectMethod(text, toStringMethod);
        result = env->GetStringUTFChars(str, 0);
        env->DeleteLocalRef(str);
        env->DeleteLocalRef(text);
    }
    
    env->DeleteLocalRef(CharSequenceClass);
    env->DeleteLocalRef(ClipboardManagerClass);
    env->DeleteLocalRef(clipboardManager);
    env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(mInitialApplication);
    env->DeleteLocalRef(activityThreadClass);
    jvm->DetachCurrentThread();
    
    return result;
}

#define Application_OpenURL(url) ((void(*)(Il2CppString*))Il2CppGetMethodOffset(OBFUSCATE("UnityEngine.CoreModule.dll"), OBFUSCATE("UnityEngine"), OBFUSCATE("Application"), OBFUSCATE("OpenURL"), 1))(url)

void OpenKeyGeneratorURL() {
    std::string url = OBFUSCATE("https://nikumods.site/GetFreeKey.php");
    Application_OpenURL(Il2CppString::Create(url.c_str()));
}

void ShowAuthWindow() {
    if(!showAuthWindow || authSuccess) return;
    
    ImGui::SetNextWindowSize(ImVec2(720, 600), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(g_GlWidth/2 - 260, g_GlHeight/2 - 225), ImGuiCond_Always);
    
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(16/255.0f, 16/255.0f, 16/255.0f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.85f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    
    ImGui::Begin(OBFUSCATE("NIKUMODS LOGIN"), &showAuthWindow, 
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    
    float windowWidth = ImGui::GetWindowWidth();
    
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + 4), IM_COL32(30, 30, 30, 255));
    
    ImGui::SetCursorPosY(30);
    ImGui::SetCursorPosX((windowWidth - 120) / 2); // "NIKUMODS" approx width 120
    ImGui::SetWindowFontScale(1.4f);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), OBFUSCATE("NIKUMODS"));
    ImGui::SetWindowFontScale(1.0f);
    
    ImGui::SetCursorPosY(70);
    ImGui::SetCursorPosX((windowWidth - 150) / 2); // "LOGIN REQUIRED" approx width 150
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), OBFUSCATE("LOGIN REQUIRED"));
    
    float inputWidth = 400;
    float inputX = (windowWidth - inputWidth) / 2;
    ImGui::SetCursorPosY(120);
    ImGui::SetCursorPosX(inputX);
    
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(30/255.0f, 30/255.0f, 30/255.0f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(28/255.0f, 28/255.0f, 28/255.0f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(28/255.0f, 28/255.0f, 28/255.0f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(15, 10));
    
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputText(OBFUSCATE("##KEY"), inputKeyBuffer, sizeof(inputKeyBuffer), 
        ImGuiInputTextFlags_CharsNoBlank);
    
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
    
    ImGui::SetCursorPosY(185);
    
    float buttonWidth = 110;
    float spacing = 15;
    float totalButtonsWidth = (buttonWidth * 3) + (spacing * 2);
    float buttonsStartX = (windowWidth - totalButtonsWidth) / 2;
   
    ImGui::SetCursorPosX(buttonsStartX);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(36/255.0f, 36/255.0f, 36/255.0f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(40/255.0f, 40/255.0f, 40/255.0f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(40/255.0f, 40/255.0f, 40/255.0f, 1.00f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 8));
    
    if(ImGui::Button(OBFUSCATE("GET KEY"), ImVec2(buttonWidth, 40))) {
        OpenKeyGeneratorURL();
    }
    
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
    
    ImGui::SameLine(0, spacing);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(36/255.0f, 36/255.0f, 36/255.0f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(40/255.0f, 40/255.0f, 40/255.0f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(40/255.0f, 40/255.0f, 40/255.0f, 1.00f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 8));
    
    if(ImGui::Button(OBFUSCATE("PASTE"), ImVec2(buttonWidth, 40))) {
        std::string clipboard = getClipboard();
        if(!clipboard.empty()) {
            strncpy(inputKeyBuffer, clipboard.c_str(), sizeof(inputKeyBuffer) - 1);
            inputKeyBuffer[sizeof(inputKeyBuffer) - 1] = '\0';
            authMessage = "Key pasted!";
            authMessageError = false;
        } else {
            authMessage = "Clipboard empty!";
            authMessageError = true;
        }
    }
    
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
    
    // LOGIN Button
    ImGui::SameLine(0, spacing);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.9f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.45f, 0.85f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 8));
    
    if(ImGui::Button(OBFUSCATE("LOGIN"), ImVec2(buttonWidth, 40))) {
        if(strlen(inputKeyBuffer) > 0) {
            std::thread([&]() {
                std::string key(inputKeyBuffer);
                if(auth.Login(key)) {
                    authMessage = "Login successful! Welcome!";
                    authMessageError = false;
                    authSuccess = true;
                    showAuthWindow = false;
                } else {
                    authMessage = "Invalid key or HWID locked!";
                    authMessageError = true;
                }
            }).detach();
        } else {
            authMessage = "Please enter or paste a key first!";
            authMessageError = true;
        }
    }
    
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
    
    // Message display - Centered
    ImGui::SetCursorPosY(250);
    if(!authMessage.empty()) {
        float msgWidth = ImGui::CalcTextSize(authMessage.c_str()).x;
        ImGui::SetCursorPosX((windowWidth - msgWidth) / 2);
        ImVec4 textColor = authMessageError ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) : ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
        ImGui::TextColored(textColor, "%s", authMessage.c_str());
    }
    
    // Telegram link - Centered
    ImGui::SetCursorPosY(300);
    std::string telegramText = "Contact Telegram: @ur_nikuu";
    float telegramWidth = ImGui::CalcTextSize(telegramText.c_str()).x;
    ImGui::SetCursorPosX((windowWidth - telegramWidth) / 2);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", telegramText.c_str());
    
    // Instructions - Centered as block
    ImGui::SetCursorPosY(340);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    
    // Center the instruction block
    float maxInstructionWidth = 0;
    const char* instructions[] = {
        OBFUSCATE("1. Click GET KEY Button"),
        OBFUSCATE("2. Generate Key"), 
        OBFUSCATE("3. Complete Shortner"),
        OBFUSCATE("4. Paste Key & Login Enjoy")
    };
    
    for(const char* instr : instructions) {
        float w = ImGui::CalcTextSize(instr).x;
        if(w > maxInstructionWidth) maxInstructionWidth = w;
    }
    
    float instructionsStartX = (windowWidth - maxInstructionWidth) / 2;
    ImGui::SetCursorPosX(instructionsStartX);
    
    for(const char* instr : instructions) {
        ImGui::Text("%s", instr);
        if(instr != instructions[3]) {
            ImGui::SetCursorPosX(instructionsStartX);
        }
    }
    
    ImGui::PopStyleColor();
    
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

void MkbMot() {
/*if (!authSuccess) { 
    ShowAuthWindow();
    return;
}*/

// --- NikuMods clone: the A-to-Z DSA-EXPERT style menu ------------------------
NikuMenu();
}
// The aimbot / ESP / aimkill / misc controls that used to live in the old
// 500x450 "NIKUMODS MOD MENU" window are now buttons inside the cloned DSA UI
// (Ragebot tab for aimbot + aimkill, Players tab for ESP, Memory tab for
// Speed Hack / Reset Guest), so that window is gone.
//
inline EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
inline EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    eglQuerySurface(dpy, surface, EGL_WIDTH, &g_GlWidth);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_GlHeight);

    if (!g_IsSetup) {
        prevWidth = g_GlWidth;
        prevHeight = g_GlHeight;
        SetupImgui();   // style + NikuMods fonts + textures are built inside SetupImgui
        g_IsSetup = true;
    }

    ImGuiIO &io = ImGui::GetIO();
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame(g_GlWidth, g_GlHeight);
    ImGui::NewFrame();
    
    static bool isTouching = false;
    static float touchStartTime = 0.0f;

    if (ImGuiOK) {
    int touchCount = ((int (*)())(Class_Input__get_touchCount))();
    if (touchCount > 0) {
        UnityEngine_Touch_Fields touch = ((UnityEngine_Touch_Fields(*)(int))(Class_Input__GetTouch))(0);
        float reverseY = io.DisplaySize.y - touch.m_Position.fields.y;
        io.MousePos = ImVec2(touch.m_Position.fields.x, reverseY);
        if (!isTouching) {
            touchStartTime = ImGui::GetTime();
        }
        io.MouseDown[0] = true;
        isTouching = true;
    }
    else {
        if (isTouching && (ImGui::GetTime() - touchStartTime < 0.06f)) {
            io.MouseDown[0] = true;
        } else {
            io.MouseDown[0] = false;
            isTouching = false;
        }
    }
    }
                    
	UpdateResetGuest();
    DrawESP(g_GlWidth, g_GlHeight);
    MkbMot();
   
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

inline void StartGUI() {
    void *ptr_eglSwapBuffer = DobbySymbolResolver("/system/lib/libEGL.so", "eglSwapBuffers");
    if (ptr_eglSwapBuffer != nullptr) {
        DobbyHook((void *)ptr_eglSwapBuffer, (void *)hook_eglSwapBuffers, (void **)&old_eglSwapBuffers);
        LOGD("GUI started successfully");
    }
}

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string>
#include <vector>
#include <set>
#include <deque>
#include <mutex>
#include <algorithm>
#include <cstring>

// NOTE: orig_get_ShowDamageNum / hook_get_ShowDamageNum live in Struct/Hooks.h
// (pulled in via Struct/Gui.hpp).  They used to be duplicated here, which made
// Main.cpp fail to compile with "redefinition of ...".



/// HOOKS -




#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string>
#include <vector>
#include <set>
#include <deque>
#include <mutex>
#include <algorithm>
#include <cstring>

int32_t (*get_SystemPlatform)();
int32_t _get_SystemPlatform() {
    return (int32_t)2;
}

int32_t (*get_ChannelType)();
int32_t _get_ChannelType() {
    return (int32_t)0;
}

int32_t (*get_Architecture)();
int32_t _get_Architecture() {
    return (int32_t)3;
}

bool enableAntiReport = true;
std::mutex logMutex;
std::deque<std::string> cmdLogs;

const std::vector<uint32_t> BLOCKED_MSG_IDS = {431, 453, 468, 939, 1026, 1079, 1364, 642, 455, 3031, 3025, 3023, 3009, 3008, 3032, 3047, 3500, 1602, 1088, 1087, 1086, 1041, 1080};
std::set<uint32_t> blockedMsgIDs = {431, 453, 1026, 939, 1079, 1364, 642, 455, 1080};
std::set<uint8_t> blockedCmds = {29, 34, 40, 253};

bool (*orig_MatchGameClient_Send)(void* self, uint32_t messageID, void* msg, uint8_t sendOption, bool cacheMsgAnyWay) = nullptr;
bool (*orig_NetworkDetectionClient_Send)(void* self, uint32_t messageID, void* msg, uint8_t sendOption, bool cacheMsgAnyWay) = nullptr;
bool (*orig_ServiceClient2_Send)(void* self, uint8_t cmd, void* data, int32_t byteCount, uint8_t region) = nullptr;
bool (*orig_EGL_SwapBuffers)(void* dpy, void* surface) = nullptr;

typedef void* (*OnSplittedDatagram_t)(void *instance, void *splittedMsg, void **bigMsg);
OnSplittedDatagram_t orig_MatchGameClient_OnSplittedDatagram = nullptr;
OnSplittedDatagram_t orig_MobileMatchGameClient_OnSplittedDatagram = nullptr;

enum Proto : int {
    INIT = 1,
    HEARTBEAT = 2,
    MATCHMAKING = 3,
    ROOM = 14,
    RECONNECTION = 17,
    HEARTBEATRANDOMMIN = 151,
    HEARTBEATRANDOMMAX = 255
};

typedef bool (*SendMessageToLobby_t)(void* thiz, int cmdType, uint32_t subCmdType, void* message, uint8_t regionID, bool ignoreDisconnectWnd);
SendMessageToLobby_t orig_SendMessageToLobby = nullptr;
void (*old_UDPSession_Send)(void* instance, uint32_t cmd, void* data, int32_t byteCount, uint8_t sendOption);
int (*orig_GGP_Init)(bool is_low_mem, int client_type, int64_t update_flag);

struct FFAntiConfigDesc {
    monoString* region;
    bool enable;
    bool hpe_enable;
    bool ffi_enable;
    bool mtp_lite_data_enable;
};

//void (*orig_ProcessffantihackOpen)(void* _this, FFAntiConfigDesc* mtpSwitchDesc);

void AddLog(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    cmdLogs.push_back(message);
    if (cmdLogs.size() > 200) cmdLogs.pop_front();
}

void (*orig_SendGameLagLog)(void*);
void (*orig_SendGameMemoryLog)(void*);
void (*orig_StartLogGameLag)(void*);
void (*orig_LogReportCheat)(void*, uint64_t, uint32_t, void*, uint32_t*, bool, uint32_t, int32_t, monoString*, int32_t);
void (*orig_LogReportCheatInHistory)(void*, uint64_t, uint32_t, void*, uint64_t, uint32_t*, uint32_t, monoString*);
void (*orig_LogBRMatchResult)(void*, uint32_t, uint32_t, int32_t, uint32_t, uint32_t, int32_t);
void (*orig_SendUGCOfflineMatchStats)(void*, void*, void*, void*);
void (*orig_SendRematchFinishLog)(void*);

int GetSystemPhysicalMemory() {
    return 10000;
}

int (*orig_GetSystemPhysicalMemory)();

void* (*orig_strstr)(const char*, const char*);
void* new_strstr(const char* s1, const char* s2) {
    if(s2) {
        const char* checks[] = {"DobbyHook", "DobbySymbolResolver", "MSJavaHookMethod", "/init.magisk.rc", "dobby", "magisk", "substrate", "xposed", "frida"};
        for(auto& check : checks) {
            if(strstr(s2, check)) {
                return NULL;
            }
        }
    }
    return orig_strstr(s1, s2);
}

bool hk_MatchGameClient_Send(void* instance, uint32_t messageID, void* msg, uint8_t sendOption, bool cacheMsgAnyWay) {
    if (enableAntiReport && std::find(BLOCKED_MSG_IDS.begin(), BLOCKED_MSG_IDS.end(), messageID) != BLOCKED_MSG_IDS.end()) {
        return false;
    }
    if (orig_MatchGameClient_Send) {
        return orig_MatchGameClient_Send(instance, messageID, msg, sendOption, cacheMsgAnyWay);
    }
    return false;
}

bool hook_NetworkDetectionClient_Send(void* self, uint32_t messageID, void* msg, uint8_t sendOption, bool cacheMsgAnyWay) {
    if (enableAntiReport && std::find(BLOCKED_MSG_IDS.begin(), BLOCKED_MSG_IDS.end(), messageID) != BLOCKED_MSG_IDS.end()) {
        return false;
    }
    if (orig_NetworkDetectionClient_Send) {
        return orig_NetworkDetectionClient_Send(self, messageID, msg, sendOption, cacheMsgAnyWay);
    }
    return false;
}

bool hook_ServiceClient2_Send(void* self, uint8_t cmd, void* data, int32_t byteCount, uint8_t region) {
    if (enableAntiReport && blockedCmds.find(cmd) != blockedCmds.end()) {
        return true;
    }
    if (orig_ServiceClient2_Send) {
        return orig_ServiceClient2_Send(self, cmd, data, byteCount, region);
    }
    return false;
}

void* hook_OnSplittedDatagram(void *instance, void *splittedMsg, void **bigMsg) {
    uint32_t cmd = *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(splittedMsg) + 0x18);
    if (std::find(BLOCKED_MSG_IDS.begin(), BLOCKED_MSG_IDS.end(), cmd) != BLOCKED_MSG_IDS.end()) {
        *bigMsg = nullptr;
        return nullptr;
    }
    if (orig_MatchGameClient_OnSplittedDatagram) {
        return orig_MatchGameClient_OnSplittedDatagram(instance, splittedMsg, bigMsg);
    } else if (orig_MobileMatchGameClient_OnSplittedDatagram) {
        return orig_MobileMatchGameClient_OnSplittedDatagram(instance, splittedMsg, bigMsg);
    }
    return nullptr;
}

bool Hook_SendMessageToLobby(void* thiz, int cmdType, uint32_t subCmdType, void* message, uint8_t regionID, bool ignoreDisconnectWnd) {
    if (enableAntiReport) {
        if (cmdType == 29 || cmdType == 34 || cmdType == 40 || cmdType == 253) {
            return true;
        }
        if (cmdType == INIT || cmdType == HEARTBEAT || cmdType == RECONNECTION || (cmdType >= HEARTBEATRANDOMMIN && cmdType <= HEARTBEATRANDOMMAX)) {
            return orig_SendMessageToLobby(thiz, cmdType, subCmdType, message, regionID, ignoreDisconnectWnd);
        }
        if (cmdType == ROOM) {
            return orig_SendMessageToLobby(thiz, cmdType, subCmdType, message, regionID, ignoreDisconnectWnd);
        }
        if (cmdType == MATCHMAKING) {
            return orig_SendMessageToLobby(thiz, cmdType, subCmdType, message, regionID, ignoreDisconnectWnd);
        }
        return true;
    }
    return orig_SendMessageToLobby(thiz, cmdType, subCmdType, message, regionID, ignoreDisconnectWnd);
}

void UDPSession_Send_Hook(void* instance, uint32_t cmd, void* data, int32_t byteCount, uint8_t sendOption) {
    if (cmd == 1079 || cmd == 1364 || cmd == 431 || cmd == 453 || cmd == 1026) {
        return;
    }
    return old_UDPSession_Send(instance, cmd, data, byteCount, sendOption);
}

int my_GGP_Init(bool is_low_mem, int client_type, int64_t update_flag) {
    if (update_flag == 2147483645 || client_type == 2) {
        return 0;
    }
    return orig_GGP_Init(is_low_mem, client_type, update_flag);
}
/*
void my_ProcessffantihackOpen(void* _this, FFAntiConfigDesc* mtpSwitchDesc) {
    if (mtpSwitchDesc) {
        mtpSwitchDesc->enable = 0;
        mtpSwitchDesc->hpe_enable = 0;
        mtpSwitchDesc->ffi_enable = 0;
        mtpSwitchDesc->mtp_lite_data_enable = 0;
    }
    orig_ProcessffantihackOpen(_this, mtpSwitchDesc);
}*/

enum Type : int32_t {
    Type_ANTIHACK_MATCH_BANNED1 = 53,
    Type_ANTIHACK_MATCH_BANNED2 = 54,
    Type_ANTIHACK_MATCH_BANNED3 = 55,
    Type_CREATE_PUNISH_MATCH_BEHAVIOR = 92,
    Type_CREATE_PUNISH_CHAT_REPORT = 93,
    Type_ESPORTS_ANTIHACK = 113
};

bool (*orig_GameFacade_Send)(uint32_t messageID, void* msg, uint8_t sendOption, bool cacheMsgAnyWay);
void (*orig_MatchGameClient_OnSplittedDatagramOld)(void* _this, void* msg);

bool my_GameFacade_Send(uint32_t messageID, void* msg, uint8_t sendOption, bool cacheMsgAnyWay) {
    if (messageID == Type_ANTIHACK_MATCH_BANNED1 || messageID == Type_ANTIHACK_MATCH_BANNED2 || messageID == Type_ANTIHACK_MATCH_BANNED3 || messageID == Type_CREATE_PUNISH_MATCH_BEHAVIOR || messageID == Type_CREATE_PUNISH_CHAT_REPORT || messageID == Type_ESPORTS_ANTIHACK) {
        return false;
    }
    return orig_GameFacade_Send(messageID, msg, sendOption, cacheMsgAnyWay);
}

bool my_MatchGameClient_Send(void* _this, uint32_t messageID, void* msg, uint8_t sendOption, bool cacheMsgAnyWay) {
    if (messageID == Type_ANTIHACK_MATCH_BANNED1 || messageID == Type_ANTIHACK_MATCH_BANNED2 || messageID == Type_ANTIHACK_MATCH_BANNED3 || messageID == Type_CREATE_PUNISH_MATCH_BEHAVIOR || messageID == Type_CREATE_PUNISH_CHAT_REPORT || messageID == Type_ESPORTS_ANTIHACK) {
        return false;
    }
    return orig_MatchGameClient_Send(_this, messageID, msg, sendOption, cacheMsgAnyWay);
}

void* my_MatchGameClient_OnSplittedDatagram(void* _this, void* msg, void** bigMsg) {
    if (!msg) return nullptr;
    uint32_t cmd = *(uint32_t*)((uintptr_t)msg + 0x18);
    if (cmd == Type_ANTIHACK_MATCH_BANNED1 || cmd == Type_ANTIHACK_MATCH_BANNED2 || cmd == Type_ANTIHACK_MATCH_BANNED3 || cmd == Type_CREATE_PUNISH_MATCH_BEHAVIOR || cmd == Type_CREATE_PUNISH_CHAT_REPORT || cmd == Type_ESPORTS_ANTIHACK) {
        *bigMsg = nullptr;
        return nullptr;
    }
    return orig_MatchGameClient_OnSplittedDatagram(_this, msg, bigMsg);
}

void my_MatchGameClient_OnSplittedDatagramOld(void* _this, void* msg) {
    if (!msg) return;
    uint32_t cmd = *(uint32_t*)((uintptr_t)msg + 0x18);
    if (cmd == Type_ANTIHACK_MATCH_BANNED1 || cmd == Type_ANTIHACK_MATCH_BANNED2 || cmd == Type_ANTIHACK_MATCH_BANNED3 || cmd == Type_CREATE_PUNISH_MATCH_BEHAVIOR || cmd == Type_CREATE_PUNISH_CHAT_REPORT || cmd == Type_ESPORTS_ANTIHACK) {
        return;
    }
    orig_MatchGameClient_OnSplittedDatagramOld(_this, msg);
}


void (*orig_ProcessBugReport)(void *instance, void *loginRes);
void hook_ProcessBugReport(void *instance, void *loginRes) {
    return;
}
/*

void (*orig_ProcessffantihackOpen)(void *instance, void *mtpSwitchDesc);
void hook_ProcessffantihackOpen(void *instance, void *mtpSwitchDesc) {
    return;
}*/

bool (*orig_GetffantihackEnabled)(void *instance);
bool hook_GetffantihackEnabled(void *instance) {
    return false;
}

void (*orig_ProcessffantihackGGP)(void *instance, void *loginRes);
void hook_ProcessffantihackGGP(void *instance, void *loginRes) {
    return;
}

#define offset_UpdateLocalMark Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIMapContentController"), OBFUSCATE("UpdateLocalMark"), 4)
bool libLoaded = false;
uintptr_t findLibrary(const char *library) {
    char filename[0xFF] = {0},
            buffer[1024] = {0};
    FILE *fp = NULL;
    uintptr_t address = 0;
    sprintf(filename, OBFUSCATE("/proc/self/maps"));
    fp = fopen(filename, OBFUSCATE("rt"));
    if (fp == NULL) {
        perror(OBFUSCATE("fopen"));
        goto done;
    }
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (strstr(buffer, library)) {
            address = (uintptr_t)strtoul(buffer, NULL, 16);
            goto done;
        }
    }
    done:
    if (fp) {
        fclose(fp);
    }
    return address;
}

uintptr_t libBase = 0; 
uintptr_t getAbsoluteAddress(const char *libraryName, uintptr_t relativeAddr) {
    libBase = findLibrary(libraryName);
    if (libBase == 0)
        return 0;
    return (libBase + relativeAddr);
}
void hack_thread() {
    sleep(10);
    pid_t pid = getpid();
    LOGD("Hack Thread initiated for PID: %i", pid);
    do {
        il2cpp_base = get_module_base(pid, "libil2cpp.so");
        sleep(3);
    } while (il2cpp_base == 0);
    LOGD("libil2cpp.so found at address: 0x%" PRIxPTR, il2cpp_base);
    Il2CppAttach();
	
	ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("GCommon"), OBFUSCATE("PlatformUtility_Android"), OBFUSCATE("GetSignature"), 0), "00 00 80 D2 C0 03 5F D6");

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("GCommon.Checker"), OBFUSCATE("GDBKJLNCNAM"), OBFUSCATE("AEPOBDBANEE"), 0), "00 00 80 D2 C0 03 5F D6");
ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("GCommon.Checker"), OBFUSCATE("GDBKJLNCNAM"), OBFUSCATE("FMCDMGAICOB"), 0), "00 00 80 D2 C0 03 5F D6");
ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("GCommon.Checker"), OBFUSCATE("GDBKJLNCNAM"), OBFUSCATE("DJGOEFJOICP"), 0), "00 00 80 D2 C0 03 5F D6");

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("GCommon.Checker"), OBFUSCATE("JNAOHDFAEPD"), OBFUSCATE("FMCDMGAICOB"), 0), "00 00 80 D2 C0 03 5F D6");
	///ALL FKING BYPASS MF -
   /* 
	DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("Build"), OBFUSCATE("get_IsHPE"), 0),
        (void*)hook_get_IsHPE, (void**)&orig_get_IsHPE);
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("GCommon.dll"), OBFUSCATE("GCommon.Checker"), OBFUSCATE("COIFGGAINCK"), OBFUSCATE("KHFLCCDDNHM"), 0),
        (void*)hook_KCMDEGKOPGO_IOFCMMGIHBB, (void**)&orig_KCMDEGKOPGO_IOFCMMGIHBB);
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("GCommon.dll"), OBFUSCATE("GCommon"), OBFUSCATE("PlatformUtility"), OBFUSCATE("Is64BitsProcess"), 0),
        (void*)hook_Is64BitsProcess, (void**)&orig_Is64BitsProcess);
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("GCommon.dll"), OBFUSCATE("GCommon"), OBFUSCATE("PlatformUtility"), OBFUSCATE("Is32BitsProcess"), 0),
        (void*)hook_Is32BitsProcess, (void**)&orig_Is32BitsProcess);
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("GooglePlayInstant"), OBFUSCATE("PlaySignatureVerifier"), OBFUSCATE("VerifyGooglePlayServices"), 1),
        (void*)hook_VerifyGooglePlayServices, (void**)&orig_VerifyGooglePlayServices);

    // P7-34. PlaySignatureVerifier.VerifyGooglePlayPackage — always pass (static)
    DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("GooglePlayInstant"), OBFUSCATE("PlaySignatureVerifier"), OBFUSCATE("VerifyGooglePlayPackage"), 2),
        (void*)hook_VerifyGooglePlayPackage, (void**)&orig_VerifyGooglePlayPackage);

    // P7-35. PlaySignatureVerifier..cctor — neutralize static constructor
    MemoryPatch::createWithHex((uintptr_t)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("GooglePlayInstant"), OBFUSCATE("PlaySignatureVerifier"), OBFUSCATE(".cctor"), 0), RET).Modify();
        
    
//PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll&qu
PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW","EventLogger","LogReportCheat",9));
    PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW","EventLogger","LogReportCheatInHistory",7));
    
    PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW","UIModelLogin","ProcessBugReport",4));
MemoryPatch::createWithHex((uintptr_t)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelLogin"), OBFUSCATE("ProcessAH"), 4), RET).Modify();
DobbyHook(Il2CppGetMethodOffset(OBFUSCATE("mscorlib.dll"), OBFUSCATE("Mono.Security.X509"), OBFUSCATE("X509Certificate"), OBFUSCATE("Parse"), 1), (void *) &_X509_Parse, (void **) &X509_Parse);
DobbyHook(Il2CppGetMethodOffset(OBFUSCATE("mscorlib.dll"), OBFUSCATE("Mono.Security.X509"), OBFUSCATE("PKCS12"), OBFUSCATE("Decrypt"), 1), (void *) &_PKCS12_Decrypt, (void **) &PKCS12_Decrypt);
DobbyHook(Il2CppGetMethodOffset(OBFUSCATE("mscorlib.dll"), OBFUSCATE("Mono.Security"), OBFUSCATE("ASN1"), OBFUSCATE("Decode"), 1), (void *) &_ASN1_Decode, (void **) &ASN1_Decode);
DobbyHook(Il2CppGetMethodOffset(OBFUSCATE("mscorlib.dll"), OBFUSCATE("Mono.Math"), OBFUSCATE("BigInteger"), OBFUSCATE("IsProbablePrime"), 0), (void *) &_BigInteger_IsPrime, (void **) &BigInteger_IsPrime);
DobbyHook(Il2CppGetMethodOffset(OBFUSCATE("mscorlib.dll"), OBFUSCATE("Mono.Security.Cryptography"), OBFUSCATE("RSAManaged"), OBFUSCATE("EncryptValue"), 1), (void *) &_RSA_EncryptValue, (void **) &RSA_EncryptValue);
DobbyHook(Il2CppGetMethodOffset(OBFUSCATE("mscorlib.dll"), OBFUSCATE("Mono.Security.Cryptography"), OBFUSCATE("RSAManaged"), OBFUSCATE("DecryptValue"), 1), (void *) &_RSA_DecryptValue, (void **) &RSA_DecryptValue);
DobbyHook(Il2CppGetMethodOffset(OBFUSCATE("mscorlib.dll"), OBFUSCATE("Mono.Security.Cryptography"), OBFUSCATE("DSAManaged"), OBFUSCATE("VerifySignature"), 2), (void *) &_DSA_VerifySignature, (void **) &DSA_VerifySignature);
DobbyHook(Il2CppGetMethodOffset(OBFUSCATE("mscorlib.dll"), OBFUSCATE("Mono.Security.Cryptography"), OBFUSCATE("PKCS1"), OBFUSCATE("Verify_v15"), 4), (void *) &_PKCS1_Verify_v15, (void **) &PKCS1_Verify_v15);


#define PATCH_RET(addr) MemoryPatch::createWithHex((uintptr_t)(addr), OBFUSCATE("C0 03 5F D6")).Modify()
#define PATCH_RET_TRUE(addr) MemoryPatch::createWithHex((uintptr_t)(addr), OBFUSCATE("20 00 80 D2 C0 03 5F D6")).Modify()
#define PATCH_RET_FALSE(addr) MemoryPatch::createWithHex((uintptr_t)(addr), OBFUSCATE("00 00 80 D2 C0 03 5F D6")).Modify()








// DOBBY HOOK + MEM PATCH FOR il2cppattach 


DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("COW"),OBFUSCATE("EventLogger"),OBFUSCATE("SendGameLagLog"),0),(void*)(+[](void*_this){return;}),(void**)&orig_SendGameLagLog);
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("COW"),OBFUSCATE("EventLogger"),OBFUSCATE("SendGameMemoryLog"),0),(void*)(+[](void*_this){return;}),(void**)&orig_SendGameMemoryLog);
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("COW"),OBFUSCATE("EventLogger"),OBFUSCATE("StartLogGameLag"),0),(void*)(+[](void*_this){return;}),(void**)&orig_StartLogGameLag);
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("COW"),OBFUSCATE("EventLogger"),OBFUSCATE("LogReportCheat"),9),(void*)(+[](void*_this,uint64_t,uint32_t,void*,uint32_t*,bool,uint32_t,int32_t,monoString*,int32_t){return;}),(void**)&orig_LogReportCheat);
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("COW"),OBFUSCATE("EventLogger"),OBFUSCATE("LogReportCheatInHistory"),7),(void*)(+[](void*_this,uint64_t,uint32_t,void*,uint64_t,uint32_t*,uint32_t,monoString*){return;}),(void**)&orig_LogReportCheatInHistory);
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("COW"),OBFUSCATE("EventLogger"),OBFUSCATE("LogBRMatchResult"),6),(void*)(+[](void*_this,uint32_t,uint32_t,int32_t,uint32_t,uint32_t,int32_t){return;}),(void**)&orig_LogBRMatchResult);
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("COW"),OBFUSCATE("EventLogger"),OBFUSCATE("SendUGCOfflineMatchStats"),4),(void*)(+[](void*_this,void*,void*,void*){return;}),(void**)&orig_SendUGCOfflineMatchStats);
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("COW"),OBFUSCATE("EventLogger"),OBFUSCATE("SendRematchFinishLog"),0),(void*)(+[](void*_this){return;}),(void**)&orig_SendRematchFinishLog);

DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("GCommon.dll"),OBFUSCATE("GCommon"),OBFUSCATE("PlatformUtility"),OBFUSCATE("GetSystemPhysicalMemory"),0),(void*)GetSystemPhysicalMemory,(void**)&orig_GetSystemPhysicalMemory);

DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("COW"),OBFUSCATE("GameFacade"),OBFUSCATE("Send"),4),(void*)my_GameFacade_Send,(void**)&orig_GameFacade_Send);
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("GCommon.dll"),OBFUSCATE("GCommon"),OBFUSCATE("MatchGameClient"),OBFUSCATE("Send"),4),(void*)my_MatchGameClient_Send,(void**)&orig_MatchGameClient_Send);
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("GCommon.dll"),OBFUSCATE("GCommon"),OBFUSCATE("MatchGameClient"),OBFUSCATE("OnSplittedDatagram"),2),(void*)my_MatchGameClient_OnSplittedDatagram,(void**)&orig_MatchGameClient_OnSplittedDatagram);
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("GCommon.dll"),OBFUSCATE("GCommon"),OBFUSCATE("MatchGameClient"),OBFUSCATE("OnSplittedDatagramOld"),1),(void*)my_MatchGameClient_OnSplittedDatagramOld,(void**)&orig_MatchGameClient_OnSplittedDatagramOld);

DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("COW.UIModelLogin"),OBFUSCATE("ProcessBugReport"),OBFUSCATE("CHMELGCOIGN"),4),(void*)hook_ProcessBugReport,(void**)&orig_ProcessBugReport);
//
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("COW.UIModelLogin"),OBFUSCATE("GetffantihackEnabled"),OBFUSCATE(""),0),(void*)hook_GetffantihackEnabled,(void**)&orig_GetffantihackEnabled);
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("COW.UIModelLogin"),OBFUSCATE("ProcessffantihackGGP"),OBFUSCATE("CHMELGCOIGN"),4),(void*)hook_ProcessffantihackGGP,(void**)&orig_ProcessffantihackGGP);

DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("GCommon"),OBFUSCATE("MatchGameClient"),OBFUSCATE("Send"),4),(void*)hk_MatchGameClient_Send,(void**)&orig_MatchGameClient_Send);
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("GCommon"),OBFUSCATE("NetworkDetectionClient"),OBFUSCATE("Send"),4),(void*)hook_NetworkDetectionClient_Send,(void**)&orig_NetworkDetectionClient_Send);
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("COW"),OBFUSCATE("ServiceClient2"),OBFUSCATE("Send"),4),(void*)hook_ServiceClient2_Send,(void**)&orig_ServiceClient2_Send);


DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("GCommon"),OBFUSCATE("MatchGameClient"),OBFUSCATE("OnSplittedDatagram"),2),(void*)hook_OnSplittedDatagram,(void**)&orig_MatchGameClient_OnSplittedDatagram);
DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("GCommon"),OBFUSCATE("MobileMatchGameClient"),OBFUSCATE("OnSplittedDatagram"),2),(void*)hook_OnSplittedDatagram,(void**)&orig_MobileMatchGameClient_OnSplittedDatagram);

DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("COW"),OBFUSCATE("ServiceConnectionManager"),OBFUSCATE("SendMessageToLobby"),5),(void*)Hook_SendMessageToLobby,(void**)&orig_SendMessageToLobby);


DobbyHook(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("Build"), OBFUSCATE("get_SystemPlatform"), 0), (void *) &_get_SystemPlatform, (void **) &get_SystemPlatform);
    DobbyHook(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("Build"), OBFUSCATE("get_ChannelType"), 0), (void *) &_get_ChannelType, (void **) &get_ChannelType);
    DobbyHook(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("Build"), OBFUSCATE("get_Architecture"), 0), (void *) &_get_Architecture, (void **) &get_Architecture);

/*
    DobbyHook(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelLogin"), OBFUSCATE("GetffantihackEnabledTempValue"), 1), (void*)my_GetffantihackEnabledTempValue, (void**)&orig_GetffantihackEnabledTempValue);
DobbyHook(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelLogin"), OBFUSCATE("ProcessffantihackOpen"), 1), (void*)my_ProcessffantihackOpen, (void**)&orig_ProcessffantihackOpen);


    
    // -------- REPORT / CHEAT --------
    PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW","UIHudObserverInteractionController","OnReportCheatSent",1));
    
    PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW","EventLogger","LogReportCheat",9));
    PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW","EventLogger","LogReportCheatInHistory",7));
    
    PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW","UIModelLogin","ProcessBugReport",4));

    // -------- PERFORMANCE / LOGS --------
    PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW","EventLogger","SendGameLagLog",0));
    PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW","EventLogger","SendGameMemoryLog",0));
    PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW","EventLogger","StartLogGameLag",0));
    PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW","EventLogger","SendUGCOfflineMatchStats",4));
    PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW","EventLogger","SendRematchFinishLog",0));

    PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW","EventLogger","SendEventLog",2));
    PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW","EventLogger","SendEventLogJson",2));
    PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW","EventLogger","SendNetworkEventLog",2));

    // -------- PERFORMANCE MONITOR --------
    PATCH_RET_TRUE(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW","PerformacneMonitor","IsLatestBattleGameHighFrameRateNormal",0));
    PATCH_RET_TRUE(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW","PerformacneMonitor","IsLatestBattleGameGraphicsQualityNormal",0));
    PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW","PerformacneMonitor","LogBattleGamePerformance",4));

    // -------- ANTICHEAT SYSTEM --------
 //   PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW.UIModelLogin","ProcessffantihackOpen","FFAntiConfigDesc",4));
    PATCH_RET_FALSE(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW.UIModelLogin","GetffantihackEnabled","",0));
  //  PATCH_RET(Il2CppGetMethodOffset("Assembly-CSharp.dll","COW.UIModelLogin","ProcessffantihackGGP","CHMELGCOIGN",4));

    // -------- CERTIFICATE BYPASS --------
    PATCH_RET_TRUE(Il2CppGetMethodOffset("mscorlib.dll","Mono.Security.X509","X509Certificate","VerifySignature",0));
    PATCH_RET_FALSE(Il2CppGetMethodOffset("mscorlib.dll","Mono.Security.X509","X509Certificate","get_IsSelfSigned",0));
    PATCH_RET_FALSE(Il2CppGetMethodOffset("mscorlib.dll","Mono.Security.X509","X509Certificate","get_IsCurrent",0));
    
    PATCH_RET(Il2CppGetMethodOffset("mscorlib.dll","Mono.Security.X509","X509Chain","Build",0));
    PATCH_RET_FALSE(Il2CppGetMethodOffset("mscorlib.dll","Mono.Security.X509","X509Chain","get_Status",0));
//==================SIGNATURE NEW
    
    // -------- PKCS12 --------
PATCH_RET(Il2CppGetMethodOffset("Mono.Security.dll","Mono.Security.X509","PKCS12","Decode",1));
PATCH_RET(Il2CppGetMethodOffset("Mono.Security.dll","Mono.Security.X509","PKCS12","Decrypt",4));
PATCH_RET(Il2CppGetMethodOffset("Mono.Security.dll","Mono.Security.X509","PKCS12","Decrypt",1));
PATCH_RET(Il2CppGetMethodOffset("Mono.Security.dll","Mono.Security.X509","PKCS12","Encrypt",4));
PATCH_RET(Il2CppGetMethodOffset("Mono.Security.dll","Mono.Security.X509","PKCS12","GetBytes",0));

// -------- X509 CHAIN / VERIFY --------
PATCH_RET_TRUE(Il2CppGetMethodOffset("Mono.Security.dll","Mono.Security.X509","X509Chain","Build",1));
PATCH_RET(Il2CppGetMethodOffset("Mono.Security.dll","Mono.Security.X509","X509Chain","IsValid",1));
PATCH_RET(Il2CppGetMethodOffset("Mono.Security.dll","Mono.Security.X509","X509Chain","IsTrusted",1));

// -------- X509 CRL --------
PATCH_RET_TRUE(Il2CppGetMethodOffset("Mono.Security.dll","Mono.Security.X509","X509Crl","VerifySignature",1));
PATCH_RET_TRUE(Il2CppGetMethodOffset("Mono.Security.dll","Mono.Security.X509","X509Crl","VerifySignature",1));
PATCH_RET_TRUE(Il2CppGetMethodOffset("Mono.Security.dll","Mono.Security.X509","X509Crl","VerifySignature",1));

// -------- X509 CERTIFICATE --------
PATCH_RET_TRUE(Il2CppGetMethodOffset("Mono.Security.dll","Mono.Security.X509","X509Certificate","VerifySignature",1));
PATCH_RET_TRUE(Il2CppGetMethodOffset("Mono.Security.dll","Mono.Security.X509","X509Certificate","VerifySignature",1));
PATCH_RET_TRUE(Il2CppGetMethodOffset("Mono.Security.dll","Mono.Security.X509","X509Certificate","VerifySignature",1));

PATCH_RET_FALSE(Il2CppGetMethodOffset("Mono.Security.dll","Mono.Security.X509","X509Certificate","get_IsCurrent",0));
PATCH_RET_FALSE(Il2CppGetMethodOffset("Mono.Security.dll","Mono.Security.X509","X509Certificate","get_IsSelfSigned",0));
    





    
ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("HuntingGroundRoundInfoData"), OBFUSCATE("ParseData"), 4), FAL);
	
ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("HuntingGroundRoundInfoData"), OBFUSCATE("GetPrimaryKey"), 0), FAL);
	
 ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("FFVoice"), OBFUSCATE("HuntingGroundRoundInfoData"), OBFUSCATE(".ctor"), 0), FAL);

 ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE(""), OBFUSCATE("EventTypeAngelDevilBubbleClick"), OBFUSCATE(".ctor"), 0), FAL);
 
  ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE(""), OBFUSCATE("SpecialKillEffectInfo"), OBFUSCATE(".ctor"), 0), FAL);
  
   ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE(""), OBFUSCATE("EventTypeSpecialKill"), OBFUSCATE(".ctor"), 0), FAL);
 

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE(""), OBFUSCATE("EventTypeBannedPlayerAppeal"), OBFUSCATE(".ctor"), 0), FAL);


ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE(""), OBFUSCATE("EventTypeAddMatchMakingBlacklist"), OBFUSCATE(".ctor"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE(""), OBFUSCATE("EventTypeRemoveBlacklist"), OBFUSCATE(".ctor"), 0), TRU);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE(""), OBFUSCATE("EventTypePotentiaBDimensionShare"), OBFUSCATE(".ctor"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE(""), OBFUSCATE("EventTypeTechBuildVote"), OBFUSCATE(".ctor"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE(""), OBFUSCATE("EventTypeUGCUnlockOfflineMode"), OBFUSCATE(".ctor"), 0), FAL);


ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE(""), OBFUSCATE("EventTypeUGCPlayerEnterOfflineMode"), OBFUSCATE(".ctor"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE(""), OBFUSCATE("EventTypeUGCOfflineMatchStats"), OBFUSCATE(".ctor"), 0), TRU);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE(""), OBFUSCATE("UGCPlatformStartSurveySubmitDetail"), OBFUSCATE(".ctor"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("GameFacade"), OBFUSCATE("DebugFFI"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("GameFacade"), OBFUSCATE("DebugBRAntiAddiction"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("BaseHighlightInfo"), OBFUSCATE("CorrectionKillEvent"), 3), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("BaseHighlightInfo"), OBFUSCATE(".ctor"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ReplayHighlightEvent"), OBFUSCATE("Clone"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ReplayHighlightEvent"), OBFUSCATE("GetDelay"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ReplayHighlightEvent"), OBFUSCATE("GetDelays"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ReplayHighlightEvent"), OBFUSCATE("GetLogString"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ReplayHighlightEvent"), OBFUSCATE(".ctor"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ReplayHighlightEventPeriod"), OBFUSCATE("Clone"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ReplayHighlightEventPeriod"), OBFUSCATE(".ctor"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ReplayKillEvent"), OBFUSCATE(".ctor"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ReplayDeadEvent"), OBFUSCATE(".ctor"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIInGameScene"), OBFUSCATE("OnEnterOrLeaveTriggerDetail"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIInGameScene"), OBFUSCATE("AddForbidPlayer"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIInGameScene"), OBFUSCATE("RemoveForbidPlayer"), 1), TRU);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIInGameScene"), OBFUSCATE("IsContainsForbidedPlayer"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE(""), OBFUSCATE("ReportData"), OBFUSCATE(".ctor"), 0), FAL);

 
ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelCustomRoom"), OBFUSCATE("RequestKickPlayer"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelCustomRoom"), OBFUSCATE("ClearData"), 1), TRU);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelCustomRoom"), OBFUSCATE("IsTimeoutOfUGCRoomMatch"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelCustomRoom"), OBFUSCATE("RequestDropMatch"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelCustomRoom"), OBFUSCATE("OnRoomAfkPunishNtfFromTeam"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelCustomRoom"), OBFUSCATE("OnRoomAfkPunishNtf"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelCustomRoom"), OBFUSCATE("get_ADCSEnabled"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelCustomRoom"), OBFUSCATE("set_ADCSEnabled"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelCustomRoom"), OBFUSCATE("get_AdCSEcoRound"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelCustomRoom"), OBFUSCATE("set_AdCSEcoRound"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("get_MVPPlayerUID"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("set_MVPPlayerUID"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("get_RecommendCriticalData"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("set_RecommendCriticalData"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("get_IsKickOutByUGC"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("set_IsKickOutByUGC"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("get_MatchBlackList"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("get_MaxMatchBlacklistCount"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("updateLeaveGame"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("ProcessMatchBlackListParam"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("RequestMatchBlackList"), 4), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("IsPlayerInBlackList"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("GetCurMatchBlackListCount"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("ProcessExpireMatchBlackList"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("ProcessRemoveMatchBlackList"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("RequestAddMatchBlackListPlayer"), 4), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("RequestRemoveMatchBlackListPlayer"), 1), TRU);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("IsOpenMatchBlackList"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("ProcessMatchFeedbackDesc"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("get_ReportedPlayerSet"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("get_Match2ReportedPlayer"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("GetCandiateReportees"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("HasAnyCandidateReporteeBeforeFilter"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("GetTeamCandidateReporteeList"), 0), FAL);


ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelMatch"), OBFUSCATE("IsKilledBySystem"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ProfileDataPunishSwitchConfig"), OBFUSCATE("Create"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ProfileDataPunishSwitchConfig"), OBFUSCATE("GetKillTimesValue"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ProfileDataPunishSwitchConfig"), OBFUSCATE("GetKillTimesText"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ProfileDataPunishSwitchConfig"), OBFUSCATE("GetHeadShotRatioValue"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ProfileDataPunishSwitchConfig"), OBFUSCATE("GetHeadShotRatioText"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ProfileDataPunishSwitchConfig"), OBFUSCATE("GetKDValue"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ProfileDataPunishSwitchConfig"), OBFUSCATE("GetKDText"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ProfileDataPunishSwitchConfig"), OBFUSCATE("Parse"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ProfileDataPunishSwitchConfig"), OBFUSCATE("FormatValue"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ProfileDataPunishSwitchConfig"), OBFUSCATE(".ctor"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("PersonalRecordData"), OBFUSCATE(".ctor"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("MatchRecordPersonal"), OBFUSCATE("get_MapName"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("MatchRecordPersonal"), OBFUSCATE(".ctor"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("MatchRecordPersonal"), OBFUSCATE("SetRatingBoardType"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("PlayerKillInfo"), OBFUSCATE(".ctor"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("AccTeamData"), OBFUSCATE(".ctor"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ResultTeamData"), OBFUSCATE(".ctor"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ResultMVPInfo"), OBFUSCATE(".ctor"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("ResultPlayerData"), OBFUSCATE(".ctor"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("PlayerBestStatsData"), OBFUSCATE(".ctor"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelSpectator"), OBFUSCATE("get_RoundResult"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelSpectator"), OBFUSCATE("AddRoundResult"), 3), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelSpectator"), OBFUSCATE("GetPlayerKillInfo"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelSpectator"), OBFUSCATE("GetPlayerOutStandingStats"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelSpectator"), OBFUSCATE("get_CurrentKillLeaderPlayerID"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelSpectator"), OBFUSCATE("set_CurrentKillLeaderPlayerID"), 1), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelSpectator"), OBFUSCATE("OnHandlePlayerKillInfo"), 3), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelSpectator"), OBFUSCATE("SortNormalMatchTeammateData"), 2), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelSpectator"), OBFUSCATE("GetMatchMvpInfo"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelSpectator"), OBFUSCATE("RecordMatchInfo"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelSpectator"), OBFUSCATE("LogToFile"), 0), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelSpectator"), OBFUSCATE("LogReplayEvent"), 5), FAL);

ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIModelSpectator"), OBFUSCATE("OnBattleDetected"), 3), FAL);

//-p20
ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIHudMatchResultItemBaseController"), OBFUSCATE(".ctor"), 0), FAL);
ZPATCH(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("UIHudMatchResultItemBaseController"), OBFUSCATE("SetMatchResultPlayerInfoData"), 3), FAL);
*/
/// end BYPASS FKING
    DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("COW.GamePlay"),OBFUSCATE("Player"),OBFUSCATE("get_InSwapWeaponCD"),0),(void*)_get_InSwapWeaponCD,(void**)&get_InSwapWeaponCD);
	DobbyHook(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW.GamePlay"), OBFUSCATE("Player"), OBFUSCATE("UpdateBehavior"), 2), (void*)_LateUpdate, (void**)&LateUpdate);
    DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW.GamePlay"), OBFUSCATE("PlayerNetwork"), OBFUSCATE("TakeDamage"), 9), (void *) &hook_PlayerNetwork_TakeDamage, (void **) &orig_PlayerNetwork_TakeDamage);
 	DobbyHook((void *) Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"),OBFUSCATE("COW.GamePlay"), OBFUSCATE("Player"),OBFUSCATE("IsWeaponInAutoCharge"), 0),(void *) IsWeaponInAutoCharge_Hook, (void **) &old_IsWeaponInAutoCharge);
	DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW.GamePlay"), OBFUSCATE("Player"), OBFUSCATE("get_ShowDamageNum"), 0), (void*)hook_get_ShowDamageNum, (void**)&orig_get_ShowDamageNum);                          
    dobbyhooks();
    StartGUI();
    ImGuiOK = true;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    jvm = vm;
    std::thread(hack_thread).detach();
    return JNI_VERSION_1_6;
}
