#include "core/World.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <iomanip>
#include <locale>

void farm::GameState::tick(Input input) { tick(tickSeconds); move(input, tickSeconds); }
void farm::GameState::move(Input input, double dt) {
    if (!std::isfinite(dt) || dt <= 0 || !std::isfinite(input.x) || !std::isfinite(input.y)) return;
    dt = std::min(dt, 0.1);
    input.x = std::clamp(input.x, -1.0, 1.0);
    input.y = std::clamp(input.y, -1.0, 1.0);
    const double length = std::hypot(input.x, input.y);
    if (length > 1) { input.x /= length; input.y /= length; }
    player_.x = std::clamp(player_.x + input.x * speed * dt, radius, columns-radius);
    player_.y = std::clamp(player_.y + input.y * speed * dt, radius, rows-radius);
}
farm::GameState::GameState() { tiles_[0].owned = true; }

const farm::Tile* farm::GameState::tile(int column, int row) const {
    if (column < 0 || column >= columns || row < 0 || row >= rows) return nullptr;
    return &tiles_[row * columns + column];
}

farm::Result farm::GameState::buyLand(int column, int row) {
    const auto* target = tile(column, row);
    if (!target) return Result::InvalidTile;
    if (target->owned) return Result::AlreadyOwned;
    bool adjacent = false;
    for (const auto offset : std::array<std::array<int, 2>, 4>{{{{1,0}},{{-1,0}},{{0,1}},{{0,-1}}}}) {
        const auto* neighbour = tile(column + offset[0], row + offset[1]);
        adjacent = adjacent || (neighbour && neighbour->owned);
    }
    if (!adjacent) return Result::NotAdjacent;
    if (balance_ < landPrice()) return Result::InsufficientFunds;
    balance_ -= landPrice();
    ++purchases_;
    tiles_[row * columns + column].owned = true;
    return Result::Success;
}

farm::Result farm::GameState::plant(int column, int row, CropId crop) {
    const auto* target = tile(column, row);
    if (!target) return Result::InvalidTile;
    if (!target->owned) return Result::NotOwned;
    if (target->planted) return Result::Occupied;
    const auto* definition = cropDefinition(crop);
    if (!definition) return Result::InvalidCrop;
    if (balance_ < plotPrice(crop)) return Result::InsufficientFunds;
    balance_ -= plotPrice(crop);
    tiles_[row * columns + column] = {true, true, 0, crop};
    return Result::Success;
}

farm::Result farm::GameState::harvest(int column, int row) {
    const auto* target = tile(column, row);
    if (!target) return Result::InvalidTile;
    if (!target->owned) return Result::NotOwned;
    if (!target->planted || target->age < cropDefinition(target->crop)->cycleSeconds) return Result::NotReady;
    credit(yield(target->crop));
    harvested_[static_cast<int>(target->crop)] = true;
    tiles_[row * columns + column].age = 0;
    return Result::Success;
}

int farm::GameState::harvestedTypes() const {
    return static_cast<int>(std::count(harvested_.begin(), harvested_.end(), true));
}

int farm::GameState::plotCount(CropId crop) const {
    return static_cast<int>(std::count_if(tiles_.begin(), tiles_.end(), [crop](const Tile& t) { return t.planted && t.crop == crop; }));
}
bool farm::GameState::automated(CropId crop) const { return cropDefinition(crop) && workers_[static_cast<int>(crop)]; }
int farm::GameState::upgradeLevel(CropId crop) const { return cropDefinition(crop) ? upgrades_[static_cast<int>(crop)] : 0; }
double farm::GameState::plotPrice(CropId crop) const {
    const auto* d = cropDefinition(crop);
    return d ? d->seedCost * std::pow(d->costGrowth, plotCount(crop)) : maxMoney;
}
double farm::GameState::workerPrice(CropId crop) const { const auto* d = cropDefinition(crop); return d ? d->seedCost * 10 : maxMoney; }
double farm::GameState::upgradePrice(CropId crop) const { const auto* d = cropDefinition(crop); return d ? d->seedCost * 8 * std::pow(1.8, upgradeLevel(crop)) : maxMoney; }
double farm::GameState::yield(CropId crop) const { const auto* d = cropDefinition(crop); return d ? d->harvestValue * std::pow(1.25, upgradeLevel(crop)) : 0; }
void farm::GameState::credit(double amount) { balance_ += std::min(amount, maxMoney - balance_); }
void farm::GameState::advance(Tile& tile, double dt) {
    if (!tile.planted) return;
    const double cycle = cropDefinition(tile.crop)->cycleSeconds;
    if (!automated(tile.crop)) { tile.age = std::min(cycle, tile.age + dt); return; }
    const double total = tile.age + dt;
    // allow for roundoff right at the end of a cycle
    const double count = std::floor((total + cycle * 1e-12) / cycle);
    tile.age = std::max(0.0, total - count * cycle);
    if (count > 0) { credit(count * yield(tile.crop)); harvested_[static_cast<int>(tile.crop)] = true; }
}
void farm::GameState::tick(double dt) {
    if (!std::isfinite(dt) || dt <= 0) return;
    // cap the calculation here; the session handles the offline time limit
    dt = std::min(dt, 315360000.0);
    tendCooldown_ = std::max(0.0, tendCooldown_ - dt);
    for (auto& tile : tiles_) advance(tile, dt);
}
farm::Result farm::GameState::hireWorker(CropId crop) {
    if (!cropDefinition(crop)) return Result::InvalidCrop;
    if (automated(crop)) return Result::AlreadyAutomated;
    if (plotCount(crop) == 0) return Result::NotReady;
    if (balance_ < workerPrice(crop)) return Result::InsufficientFunds;
    balance_ -= workerPrice(crop); workers_[static_cast<int>(crop)] = true;
    for (auto& tile : tiles_) if (tile.crop == crop) advance(tile, 0);
    return Result::Success;
}
farm::Result farm::GameState::upgrade(CropId crop) {
    if (!cropDefinition(crop)) return Result::InvalidCrop;
    if (!plotCount(crop)) return Result::NotReady;
    if (upgradeLevel(crop) >= maxUpgrade) return Result::UpgradeLimit;
    if (balance_ < upgradePrice(crop)) return Result::InsufficientFunds;
    balance_ -= upgradePrice(crop); ++upgrades_[static_cast<int>(crop)];
    return Result::Success;
}
farm::Result farm::GameState::tend(int column, int row) {
    const auto* t = tile(column,row);
    if (!t) return Result::InvalidTile;
    if (!t->planted || (!automated(t->crop) && t->age >= cropDefinition(t->crop)->cycleSeconds)) return Result::NotReady;
    if (tendCooldown_ > 1e-9) return Result::Cooldown;
    advance(tiles_[row * columns + column], cropDefinition(t->crop)->cycleSeconds * .1);
    tendCooldown_ = 5;
    return Result::Success;
}
double farm::GameState::incomeRate() const {
    double rate = 0;
    for (const auto& t : tiles_) if (t.planted && automated(t.crop)) rate += yield(t.crop) / cropDefinition(t.crop)->cycleSeconds;
    return rate;
}
std::string farm::formatMoney(double value) {
    const char* units[] = {"", "K", "M", "B", "T", "Qa", "Qi"};
    std::ostringstream out; out.imbue(std::locale::classic());
    int unit = 0;
    while (value >= 999.995 && unit < 6) { value /= 1000; ++unit; }
    if (value >= 1000) out << std::scientific << std::setprecision(2) << value * 1e18;
    else out << std::fixed << std::setprecision(2) << value << units[unit];
    return out.str();
}
std::string farm::GameState::nextGoal() const {
    int total = 0; for (const auto& t : tiles_) total += t.planted;
    const auto progress = [&](double cost) { return " ($" + formatMoney(balance_) + " / $" + formatMoney(cost) + ")"; };
    if (!total) return "Establish tomatoes" + progress(plotPrice(CropId::Tomato));
    for (int i=0;i<5;++i) { auto c=static_cast<CropId>(i); if (plotCount(c) && !automated(c)) return "Hire: " + std::string(crops[i].name) + progress(workerPrice(c)); }
    for (int i=0;i<5;++i) { auto c=static_cast<CropId>(i); if (plotCount(c) && upgradeLevel(c) < std::min(maxUpgrade, 1 + total/5)) return "Upgrade " + std::string(crops[i].name) + progress(upgradePrice(c)); }
    if (total == ownedTiles() && ownedTiles() < columns*rows) return "Buy adjacent land" + progress(landPrice());
    if (total < ownedTiles()) return "Establish another crop plot";
    int lowest=0; for (int i=1;i<5;++i) if (plotCount(static_cast<CropId>(i)) && (!plotCount(static_cast<CropId>(lowest)) || upgrades_[i]<upgrades_[lowest])) lowest=i;
    if (upgrades_[lowest] == maxUpgrade) return "All plots and yield upgrades complete!";
    return "Upgrade " + std::string(crops[lowest].name) + progress(upgradePrice(static_cast<CropId>(lowest)));
}
