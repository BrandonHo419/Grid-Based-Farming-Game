#include "storage/SaveStore.h"
#include "core/SaveCodec.h"
#include <array>
#include <cstdlib>
#include <fstream>
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace fs = std::filesystem;

fs::path farm::SaveStore::defaultPath() {
#ifdef __EMSCRIPTEN__
    return {}; // browser demos do not use desktop saves
#elif defined(_WIN32)
    const DWORD size = GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0);
    if (!size) return {};
    std::wstring value(size, L'\0');
    const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", value.data(), size);
    if (!length || length >= size) return {};
    value.resize(length);
    const fs::path base(value);
    return base.is_absolute() ? base / "Grid-Based-Farming-Game" / "idle.save" : fs::path{};
#else
    if (const char* value = std::getenv("XDG_DATA_HOME"); value && *value && fs::path(value).is_absolute())
        return fs::path(value) / "Grid-Based-Farming-Game" / "idle.save";
    if (const char* value = std::getenv("HOME"); value && *value && fs::path(value).is_absolute())
        return fs::path(value) / ".local/share/Grid-Based-Farming-Game/idle.save";
    return {};
#endif
}

bool farm::SaveStore::save(const fs::path& path, const World& world, std::string& error) {
    error.clear();
    const auto fail = [&](const char* message) { error = message; return false; };
    if (path.empty()) return fail("Save location is unavailable.");
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    if (ec) return fail("Cannot create the save folder.");
    // use a staging folder so a failed write leaves the old save intact
    auto staging = path;
    staging += ".writing";
    if (!fs::create_directory(staging, ec)) return fail("Save busy; check the .writing folder.");
    const auto temporary = staging / "farm.tmp";
    struct Cleanup {
        fs::path temporary, directory;
        ~Cleanup() {
            std::error_code ignored;
            fs::remove(temporary, ignored);
            fs::remove(directory, ignored);
        }
    } cleanup{temporary, staging};
    std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
    if (!out) return fail("Cannot write the temporary save.");
    const std::string text = SaveCodec::encode(world);
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
    out.flush();
    const bool written = static_cast<bool>(out);
    out.close();
    if (!written || out.fail()) return fail("Save write failed; old save kept.");
#ifdef _WIN32
    // replace on the same volume without deleting the old save first
    if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        return fail("Cannot replace save; old save kept.");
#else
    fs::rename(temporary, path, ec);
    if (ec) return fail("Cannot replace save; old save kept.");
#endif
    return true;
}

bool farm::SaveStore::load(const fs::path& path, World& world, std::string& error) {
    error.clear();
    if (path.empty()) { error = "Save location is unavailable."; return false; }
    std::ifstream in(path, std::ios::binary);
    if (!in) { error = "Cannot open save; save a farm first."; return false; }
    std::array<char, SaveCodec::maxBytes + 1> buffer{};
    in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    if (in.bad()) { error = "Could not read the save file."; return false; }
    return SaveCodec::decode(std::string(buffer.data(), static_cast<std::size_t>(in.gcount())), world, error);
}
