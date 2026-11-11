#pragma once
#include "core/World.h"

namespace farm {
enum class Confirmation { None, Restart, Save, Load };
// pause and confirmation state, separate from window input
class Session {
public:
    World& world() { return world_; }
    const World& world() const { return world_; }
    bool paused() const { return paused_; }
    Confirmation confirmation() const { return confirmation_; }
    bool confirming() const { return confirmation_ != Confirmation::None; }
    bool confirmingRestart() const { return confirmation_ == Confirmation::Restart; }
    bool canAct() const { return !paused_ && !confirming(); }
    void tick(Input input) { if (canAct()) world_.tick(input); }
    void togglePause() { if (!confirming()) paused_ = !paused_; }
    void requestRestart() { request(Confirmation::Restart); }
    void requestSave() { request(Confirmation::Save); }
    void requestLoad() { request(Confirmation::Load); }
    void cancelConfirmation() { confirmation_ = Confirmation::None; }
    void cancelRestart() { if (confirmingRestart()) cancelConfirmation(); }
    bool confirmRestart() {
        if (!confirmingRestart()) return false;
        world_ = World{};
        paused_ = false;
        cancelConfirmation();
        return true;
    }
    bool applyLoadedWorld(const World& world) {
        if (confirmation_ != Confirmation::Load) return false;
        world_ = world;
        paused_ = true;
        cancelConfirmation();
        return true;
    }
private:
    void request(Confirmation kind) { if (!confirming()) confirmation_ = kind; }
    World world_;
    bool paused_ = false;
    Confirmation confirmation_ = Confirmation::None;
};
}
