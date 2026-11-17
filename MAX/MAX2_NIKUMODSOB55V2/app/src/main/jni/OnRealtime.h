/*=============================================================================
  OnRealtime — realtime PATCHLIB loader
  Fetches "onrealtime.txt" from the server and applies every PATCHLIB entry
  at runtime. Format of the file (same as MAX/onrealtime.cpp):
      PATCHLIB("libanogs.so","0x1B7884","00 00 80 D2 C0 03 5F D6"); // comment
  If the server is unreachable or the file is empty, the built-in fallback
  list below is applied instead, so the mod always patches.
=============================================================================*/
#pragma once

#include <string>
#include <vector>
#include <regex>
#include <cstdlib>
#include <android/log.h>
#include <curl/curl.h>
#include <unistd.h>
#include <sys/types.h>
#include "ZaruPatch.h"
#include "Struct/tools.hpp"
#include "Toast.h"

#define ONREALTIME_TAG "OnRealtime"
#define ONREALTIME_URL "https://nikumods.site/onrealtime.txt"
// Set to 1 once the server endpoint is ready to serve onrealtime.txt.
// 0 = test mode, apply the built-in fallback list only (no network).
#define ONREALTIME_USE_NETWORK 0

struct OnRealtimePatch {
    std::string library;
    uintptr_t offset;
    std::string hex;
};

const std::vector<OnRealtimePatch> kOnRealtimeFallback = {
    { "libanogs.so", 0x1B7884, "00 00 80 D2 C0 03 5F D6" },
    { "libanogs.so", 0x1B767C, "00 00 80 D2 C0 03 5F D6" },
    { "libanogs.so", 0x1B7780, "00 00 80 D2 C0 03 5F D6" },
    { "libanogs.so", 0x1FBA00, "00 00 80 D2 C0 03 5F D6" },
    { "libanogs.so", 0x1FB890, "00 00 80 D2 C0 03 5F D6" },
    { "libanogs.so", 0x1FB6A4, "00 00 80 D2 C0 03 5F D6" },
    { "libanogs.so", 0x321ED8, "00 00 80 D2 C0 03 5F D6" },
    { "libanogs.so", 0x32199C, "00 00 80 D2 C0 03 5F D6" },
    { "libanogs.so", 0x32186C, "00 00 80 D2 C0 03 5F D6" },
};

static size_t OnRealtimeWriteCb(void* contents, size_t size, size_t nmemb, std::string* out) {
    size_t total = size * nmemb;
    out->append((char*)contents, total);
    return total;
}

inline std::string OnRealtimeDownload(const char* url) {
    std::string response;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnRealtimeWriteCb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return response;
}

inline std::vector<OnRealtimePatch> OnRealtimeParse(const std::string& content) {
    std::vector<OnRealtimePatch> out;
    std::regex re(R"r(PATCHLIB\s*\(\s*"([^"]+)"\s*,\s*"(0x[0-9a-fA-F]+)"\s*,\s*"([0-9A-Fa-f\s]+)"\s*\))r");
    for (auto it = std::sregex_iterator(content.begin(), content.end(), re);
         it != std::sregex_iterator(); ++it) {
        OnRealtimePatch p;
        p.library = (*it)[1].str();
        p.offset = (uintptr_t)strtoull((*it)[2].str().c_str(), nullptr, 16);
        p.hex = (*it)[3].str();
        out.push_back(p);
    }
    return out;
}

inline bool OnRealtimeApplyPatch(const OnRealtimePatch& p) {
    pid_t pid = getpid();
    uintptr_t base = get_module_base(pid, p.library.c_str());
    if (base == 0) {
        __android_log_print(ANDROID_LOG_ERROR, ONREALTIME_TAG, "lib base not found: %s", p.library.c_str());
        PushToast(std::string("PATCH FAIL: lib not found ") + p.library, false);
        return false;
    }
    uintptr_t addr = base + p.offset;
    if (addr && addr > 0x1000) {
        ZaruPatch(addr, p.hex).Modify();
        __android_log_print(ANDROID_LOG_DEBUG, ONREALTIME_TAG, "patched %s + 0x%lx", p.library.c_str(), (unsigned long)p.offset);
        return true;
    }
    PushToast(std::string("PATCH FAIL: bad addr ") + p.library, false);
    return false;
}

inline void ApplyRealtimePatches() {
    std::vector<OnRealtimePatch> patches;
#if ONREALTIME_USE_NETWORK
    std::string content = OnRealtimeDownload(ONREALTIME_URL);
    if (!content.empty()) {
        patches = OnRealtimeParse(content);
        __android_log_print(ANDROID_LOG_DEBUG, ONREALTIME_TAG, "fetched %zu patches from server", patches.size());
    }
#endif
    if (patches.empty()) {
#if ONREALTIME_USE_NETWORK
        __android_log_print(ANDROID_LOG_WARN, ONREALTIME_TAG, "server list empty/unreachable, using fallback");
#endif
        patches = kOnRealtimeFallback;
    }
    int ok = 0, fail = 0;
    for (const auto& p : patches) {
        if (OnRealtimeApplyPatch(p)) ok++;
        else fail++;
    }
    char summary[128];
    snprintf(summary, sizeof(summary), "PATCHLIB: %d sukses, %d gagal", ok, fail);
    PushToast(summary, fail == 0);
}
