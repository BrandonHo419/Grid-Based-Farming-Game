#include "core/SaveCodec.h"
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <vector>

std::string farm::SaveCodec::encode(const World& world) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<double>::max_digits10);
    out << "GRID_IDLE_SAVE 2\nsaved_at " << world.savedAt_ << "\nmoney " << world.balance_
        << "\npurchases " << world.purchases_ << "\nplayer "
        << world.player_.x << ' ' << world.player_.y << "\nharvested";
    for (bool harvested : world.harvested_) out << ' ' << (harvested ? 1 : 0);
    out << "\nworkers";
    for (bool worker : world.workers_) out << ' ' << worker;
    out << "\nupgrades";
    for (int level : world.upgrades_) out << ' ' << level;
    out << "\ncooldown " << world.tendCooldown_;
    out << "\ntiles\n";
    for (const auto& tile : world.tiles_) {
        out << (tile.owned ? 1 : 0) << ' ' << (tile.planted ? 1 : 0) << ' '
            << static_cast<int>(tile.crop) << ' ' << tile.age << '\n';
    }
    return out.str();
}

bool farm::SaveCodec::decode(const std::string& text, World& world, std::string& error) {
    error.clear();
    const auto fail = [&](const char* message) { error = message; return false; };
    if (text.size() > maxBytes) return fail("Save file is too large.");
    std::istringstream in(text);
    in.imbue(std::locale::classic());
    const auto tag = [&](const char* expected) {
        std::string value;
        return static_cast<bool>(in >> value) && value == expected;
    };
    int version = 0;
    if (!tag("GRID_IDLE_SAVE") || !(in >> version)) return fail("Invalid save header.");
    if (version != 1 && version != 2) return fail("Unsupported save version.");
    World candidate;
    if (version == 2 && (!tag("saved_at") || !(in >> candidate.savedAt_) || candidate.savedAt_ < 0)) return fail("Invalid save timestamp.");
    if (!tag("money") || !(in >> candidate.balance_) || !std::isfinite(candidate.balance_) || candidate.balance_ < 0 || candidate.balance_ > World::maxMoney ||
        !tag("purchases") || !(in >> candidate.purchases_) ||
        candidate.purchases_ < 0 || candidate.purchases_ >= World::columns * World::rows ||
        !tag("player") || !(in >> candidate.player_.x >> candidate.player_.y))
        return fail("Invalid farm values.");
    const auto player = candidate.player_;
    if (!std::isfinite(player.x) || !std::isfinite(player.y) ||
        player.x < World::radius || player.x > World::columns - World::radius ||
        player.y < World::radius || player.y > World::rows - World::radius)
        return fail("Invalid farmer position.");
    if (!tag("harvested")) return fail("Missing goal progress.");
    for (auto& harvested : candidate.harvested_) {
        int value = -1;
        if (!(in >> value) || (value != 0 && value != 1)) return fail("Invalid goal progress.");
        harvested = value == 1;
    }
    if (!tag("workers")) return fail("Missing workers.");
    for (auto& worker : candidate.workers_) { int v; if (!(in >> v) || (v != 0 && v != 1)) return fail("Invalid worker."); worker = v == 1; }
    if (!tag("upgrades")) return fail("Missing upgrades.");
    for (auto& level : candidate.upgrades_) if (!(in >> level) || level < 0 || level > World::maxUpgrade) return fail("Invalid upgrade.");
    if (!tag("cooldown") || !(in >> candidate.tendCooldown_) || !std::isfinite(candidate.tendCooldown_) || candidate.tendCooldown_ < 0 || candidate.tendCooldown_ > 5) return fail("Invalid cooldown.");
    if (!tag("tiles")) return fail("Missing farm tiles.");
    int ownedCount = 0;
    for (auto& tile : candidate.tiles_) {
        int owned = -1, planted = -1, crop = -1;
        double age = -1;
        if (!(in >> owned >> planted >> crop >> age) ||
            (owned != 0 && owned != 1) || (planted != 0 && planted != 1))
            return fail("Invalid farm tiles.");
        const auto id = static_cast<CropId>(crop);
        const auto* definition = cropDefinition(id);
        if (!definition || !std::isfinite(age) || age < 0 || age > definition->cycleSeconds ||
            (planted && !owned) || (!planted && (age != 0 || crop != 0)))
            return fail("Invalid crop state.");
        tile = {owned == 1, planted == 1, age, id};
        ownedCount += owned;
    }
    if (!candidate.tiles_[0].owned || ownedCount != candidate.purchases_ + 1)
        return fail("Invalid land ownership.");
    // all land must connect back to the starting tile
    std::array<bool, World::columns * World::rows> visited{};
    std::vector<int> queue{0};
    visited[0] = true;
    for (std::size_t head = 0; head < queue.size(); ++head) {
        const int cell = queue[head];
        const int x = cell % World::columns, y = cell / World::columns;
        for (const auto offset : std::array<std::array<int,2>,4>{{{{1,0}},{{-1,0}},{{0,1}},{{0,-1}}}}) {
            const int nx = x + offset[0], ny = y + offset[1];
            const auto* neighbour = candidate.tile(nx, ny);
            if (!neighbour || !neighbour->owned) continue;
            const int index = ny * World::columns + nx;
            if (!visited[index]) { visited[index] = true; queue.push_back(index); }
        }
    }
    for (int i=0;i<5;++i) if ((candidate.workers_[i] || candidate.upgrades_[i]) && !candidate.plotCount(static_cast<CropId>(i))) return fail("Upgrade or worker without plots.");
    if (static_cast<int>(queue.size()) != ownedCount) return fail("Disconnected owned land.");
    in >> std::ws;
    if (!in.eof()) return fail("Unexpected data after farm tiles.");
    world = candidate;
    return true;
}
