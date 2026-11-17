/*=============================================================================
  Toast — simple in-game notification overlay (drawn on the foreground draw
  list). Thread-safe: producers can PushToast() from any thread.
=============================================================================*/
#pragma once

#include <deque>
#include <mutex>
#include <string>
#include <chrono>
#include <imgui.h>

struct ToastMsg {
    std::string text;
    bool success;
    std::chrono::steady_clock::time_point created;
};

static std::deque<ToastMsg> g_Toasts;
static std::mutex g_ToastMutex;
static const float TOAST_DURATION_SECONDS = 4.0f;

inline void PushToast(const std::string& text, bool success = true) {
    std::lock_guard<std::mutex> lock(g_ToastMutex);
    ToastMsg t;
    t.text = text;
    t.success = success;
    t.created = std::chrono::steady_clock::now();
    g_Toasts.push_back(t);
    while (g_Toasts.size() > 4) g_Toasts.pop_front();
}

inline void DrawToasts() {
    std::lock_guard<std::mutex> lock(g_ToastMutex);
    auto now = std::chrono::steady_clock::now();
    while (!g_Toasts.empty()) {
        auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_Toasts.front().created).count() / 1000.0f;
        if (age > TOAST_DURATION_SECONDS) g_Toasts.pop_front();
        else break;
    }
    if (g_Toasts.empty()) return;

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const float screenW = ImGui::GetIO().DisplaySize.x;
    float y = 20.0f;
    for (const auto& t : g_Toasts) {
        ImVec2 textSize = ImGui::CalcTextSize(t.text.c_str());
        const float pad = 10.0f;
        float w = textSize.x + pad * 2.0f + 22.0f;
        float x = (screenW - w) * 0.5f;
        ImU32 bg = t.success ? IM_COL32(28, 120, 60, 235) : IM_COL32(150, 42, 42, 235);
        dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + textSize.y + pad * 2.0f), bg, 6.0f);
        dl->AddText(ImVec2(x + pad, y + pad), IM_COL32(255, 255, 255, 255), t.text.c_str());
        dl->AddText(ImVec2(x + pad + textSize.x + 14.0f, y + pad), IM_COL32(255, 255, 255, 255), t.success ? "OK" : "ERR");
        y += textSize.y + pad * 2.0f + 8.0f;
    }
}
