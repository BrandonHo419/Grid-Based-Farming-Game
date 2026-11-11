#pragma once
#include "core/Session.h"
#include "app/TileSelection.h"
#include "app/FarmAudio.h"
#include <string>

class Game {
public:
    explicit Game(bool preview = false, bool previewSettings = false);
    bool saveOnClose();
    void frame();
private:
#ifdef GRIDFARM_RUNTIME_CHECKS
    friend struct GameRuntimeChecks;
#endif
    farm::Session session_;
    bool helpOpen_ = false;
    bool settingsOpen_ = false;
    bool reducedMotion_ = false;
    bool preview_ = false;
    bool playerWalking_ = false;
    FarmAudio audio_;
    double feedbackTime_ = 0;
    bool feedbackSuccess_ = true;
    TileSelection feedbackTile_;
    std::string feedbackLabel_;
    bool automaticSaving_ = false;
    double autosaveElapsed_ = 0;
    double workerTime_ = 0;
    std::string welcome_;
    std::string saveStatus_;
    bool saveCurrent();
    double lastFrameTime_ = -1;
    bool focusedLastFrame_ = false;
    TileSelection selection_;
    farm::CropId selectedCrop_ = farm::CropId::Tomato;
    std::string message_ = "Select a tile, then choose an action.";
    void handleActions();
    void update(double dt, bool focused);
    void confirmAction();
    void render() const;
};
