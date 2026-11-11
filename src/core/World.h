#pragma once
#include <array>
#include <string>
#include <cstdint>

namespace farm {
struct Input { double x = 0; double y = 0; };
struct Position { double x = 0.5; double y = 0.5; };
enum class Result { Success, InvalidTile, AlreadyOwned, NotAdjacent, InsufficientFunds, NotOwned, Occupied, NotReady, InvalidCrop, BalanceLimit, AlreadyAutomated, Cooldown, UpgradeLimit };
enum class CropId { Tomato, Blueberry, Carrot, Beet, Strawberry };
struct CropDefinition { const char* name; double seedCost; double harvestValue; double cycleSeconds; double costGrowth = 1.15; };
inline constexpr std::array<CropDefinition, 5> crops{{
    {"Tomatoes", 25, 60, 10},
    {"Blueberries", 50, 120, 20},
    {"Carrots", 15, 36, 6},
    {"Beets", 35, 84, 14},
    {"Strawberries", 40, 96, 16}
}};
inline constexpr auto tomato = crops[0];
inline const CropDefinition* cropDefinition(CropId id) {
    const int index = static_cast<int>(id);
    return index >= 0 && index < static_cast<int>(crops.size()) ? &crops[index] : nullptr;
}
struct Tile {
    bool owned = false;
    bool planted = false;
    double age = 0;
    CropId crop = CropId::Tomato;
};
class GameState {
public:
    static constexpr int columns = 10;
    static constexpr int rows = 7;
    static constexpr double radius = 0.2;
    static constexpr double speed = 3.0;
    static constexpr double tickSeconds = 1.0 / 60.0;
    GameState();
    void tick(double dt);
    void move(Input input, double dt);
    Result hireWorker(CropId crop);
    Result upgrade(CropId crop);
    Result tend(int column, int row);
    int plotCount(CropId crop) const;
    bool automated(CropId crop) const;
    int upgradeLevel(CropId crop) const;
    double plotPrice(CropId crop) const;
    double workerPrice(CropId crop) const;
    double upgradePrice(CropId crop) const;
    double yield(CropId crop) const;
    double incomeRate() const;
    double tendCooldown() const { return tendCooldown_; }
    std::string nextGoal() const;
    std::int64_t savedAt() const { return savedAt_; }
    void markSaved(std::int64_t now) { if (now > savedAt_) savedAt_ = now; }
    static constexpr double maxMoney = 1e100;
    static constexpr int maxUpgrade = 100;
    void tick(Input input);
    Result buyLand(int column, int row);
    Result plant(int column, int row, CropId crop = CropId::Tomato);
    Result harvest(int column, int row);
    const Tile* tile(int column, int row) const;
    double balance() const { return balance_; }
    double landPrice() const { return 100 + purchases_ * 50; }
    int harvestedTypes() const;
    int ownedTiles() const { return purchases_ + 1; }
    bool goalsComplete() const { return harvestedTypes() == static_cast<int>(crops.size()) && ownedTiles() >= 10; }
    Position player() const { return player_; }
private:
    friend class SaveCodec;
    void advance(Tile& tile, double dt);
    void credit(double amount);
    std::array<bool, crops.size()> workers_{};
    std::array<int, crops.size()> upgrades_{};
    double tendCooldown_ = 0;
    std::int64_t savedAt_ = 0;
    Position player_;
    std::array<Tile, columns * rows> tiles_{};
    double balance_ = 1000;
    int purchases_ = 0;
    std::array<bool, crops.size()> harvested_{};
};
using World = GameState;
std::string formatMoney(double value);
}
