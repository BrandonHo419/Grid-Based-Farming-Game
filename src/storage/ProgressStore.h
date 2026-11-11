#pragma once
#include "storage/SaveStore.h"
#include "core/Offline.h"
namespace farm {
// save the catch-up result before showing it, so retrying cannot pay twice
inline bool loadProgress(const std::filesystem::path& path, World& live,
                         std::int64_t now, OfflineSummary& summary, std::string& error) {
    World candidate;
    if (now < 0) { error="System clock unavailable."; return false; }
    if (!SaveStore::load(path,candidate,error)) return false;
    const auto result=catchUp(candidate,now);
    if (!SaveStore::save(path,candidate,error)) return false;
    live=candidate; summary=result;
    return true;
}
inline bool saveProgress(const std::filesystem::path& path, World& live,
                         std::int64_t now, std::string& error) {
    if (now < 0) { error="System clock unavailable."; return false; }
    auto candidate=live; candidate.markSaved(now);
    if (!SaveStore::save(path,candidate,error)) return false;
    live=candidate; return true;
}
}
