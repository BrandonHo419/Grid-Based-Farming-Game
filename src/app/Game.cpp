#include "app/Game.h"
#include "app/UiLayout.h"
#include "app/Actions.h"
#include "raylib.h"
#include "storage/ProgressStore.h"
#include "app/WorkerVisual.h"
#include "app/FarmArt.h"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <cmath>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif


namespace {
UiLayout layoutForWindow() { return {static_cast<float>(GetScreenWidth()),static_cast<float>(GetScreenHeight())}; }
Rectangle rect(UiRect r) { return {r.x,r.y,r.width,r.height}; }
Rectangle actionButton(int i) { return rect(layoutForWindow().action(i)); }
Rectangle cropButton(int i) { return rect(layoutForWindow().crop(i)); }
Rectangle helpButton() { return {GetScreenWidth()-176.0f,16,64,28}; }
Rectangle settingsButton() { return {GetScreenWidth()-126.0f,GetScreenHeight()-48.0f,114,26}; }
Rectangle settingRow(int i) { return {GetScreenWidth()/2.0f-148,GetScreenHeight()/2.0f-100+i*50,296,36}; }
void fitted(const std::string& text,float x,float y,float width,float size,Color color) {
    const float measured=MeasureTextEx(GetFontDefault(),text.c_str(),size,1).x;
    if(measured>width) size*=width/measured;
    DrawTextEx(GetFontDefault(),text.c_str(),{x,y},size,1,color);
}
Rectangle pauseButton() { return {GetScreenWidth() - 100.0f, 16, 88, 28}; }
Rectangle dialogButton(bool confirm) {
    return {GetScreenWidth() / 2.0f - 148 + (confirm ? 0 : 156),
            GetScreenHeight() / 2.0f + 30, 140, 32};
}
bool clicked(Rectangle button) {
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), button);
}
Color cropColor(farm::CropId id) {
    switch (id) {
    case farm::CropId::Tomato: return RED;
    case farm::CropId::Blueberry: return SKYBLUE;
    case farm::CropId::Carrot: return ORANGE;
    case farm::CropId::Beet: return PURPLE;
    case farm::CropId::Strawberry: return PINK;
    }
    return WHITE;
}
const char* feedback(farm::Result result) {
    switch (result) {
    case farm::Result::AlreadyAutomated: return "Worker already hired.";
    case farm::Result::Cooldown: return "Tending recharges every five seconds.";
    case farm::Result::UpgradeLimit: return "Yield upgrade limit reached.";
    case farm::Result::Success: return "Done.";
    case farm::Result::InvalidTile: return "Choose a tile inside the grid.";
    case farm::Result::AlreadyOwned: return "You already own this tile.";
    case farm::Result::NotAdjacent: return "Buy beside owned land (not diagonally).";
    case farm::Result::InsufficientFunds: return "Not enough money.";
    case farm::Result::NotOwned: return "Buy this land first.";
    case farm::Result::Occupied: return "This tile already has a crop.";
    case farm::Result::BalanceLimit: return "Balance limit reached; crop kept.";
    case farm::Result::InvalidCrop: return "Choose a valid crop.";
    case farm::Result::NotReady: return "Requires a crop plot (ripe for harvest).";
    }
    return "Unknown action result.";
 }
const char* actionReason(farm::Result result,int action) {
    if(result==farm::Result::NotReady) {
        if(action==3 || action==4) return "Establish a plot of the selected crop first.";
        if(action==2) return "Select a ripe plot to harvest.";
        if(action==5) return "Select a growing plot; harvest ripe manual crops first.";
    }
    return feedback(result);
}
}

namespace {
std::int64_t wallTime() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
std::string welcomeText(const farm::OfflineSummary& summary) {
    if(summary.clockBackwards) return "Clock moved backwards; no offline income.";
    return "Away " + std::to_string(static_cast<int>(std::min(summary.elapsed/60,999999.0))) +
        "m | credited " + std::to_string(static_cast<int>(summary.credited)) + "s | +$" + farm::formatMoney(summary.earnings);
}
}
Game::Game(bool preview,bool previewSettings):settingsOpen_(previewSettings),preview_(preview) {
    if(preview_) {
        auto& world=session_.world();
        world.plant(0,0); world.hireWorker(farm::CropId::Tomato); world.tick(28800);
        for(int y=0;y<4;++y) for(int x=0;x<7;++x) {
            if(x==0 && y==0) continue;
            world.buyLand(x,y);
            world.plant(x,y,static_cast<farm::CropId>((x+y)%5));
            world.tick(.7);
        }
        for(int i=0;i<5;++i) world.hireWorker(static_cast<farm::CropId>(i));
        selection_={3,2}; workerTime_=2;
        saveStatus_="Preview only | no saves read or written";
        return;
    }
#ifndef __EMSCRIPTEN__
    const auto path=farm::SaveStore::defaultPath();
    std::error_code ec;
    const bool exists=!path.empty() && std::filesystem::exists(path,ec);
    if(path.empty() || ec) { saveStatus_="Save location unavailable; automatic saving disabled."; return; }
    if(!exists) { automaticSaving_=true; saveStatus_="Autosave every 30s"; return; }
    farm::World loaded; farm::OfflineSummary summary; std::string error;
    if(farm::loadProgress(path,loaded,wallTime(),summary,error)) {
        session_.requestLoad(); session_.applyLoadedWorld(loaded);
        automaticSaving_=true; welcome_=welcomeText(summary);
        message_="Welcome back. Space resumes."; saveStatus_="Loaded; autosave enabled";
    } else { saveStatus_=error+" Auto-save OFF; F5 explicitly replaces slot."; }
#else
    saveStatus_="Browser persistence is not enabled.";
#endif
}
bool Game::saveCurrent() {
    if(preview_) return true;
#ifdef __EMSCRIPTEN__
    return true;
#else
    std::string error;
    const bool saved=farm::saveProgress(farm::SaveStore::defaultPath(),session_.world(),wallTime(),error);
    saveStatus_=saved ? "Saved | autosave every 30s" : error;
    if(saved) { automaticSaving_=true; autosaveElapsed_=0; }
    return saved;
#endif
}
bool Game::saveOnClose() {
    if(!automaticSaving_) return true;
    if(saveCurrent()) return true;
    std::cerr << "Could not save on close: " << saveStatus_ << '\n';
    return false;
}

void Game::confirmAction() {
    const auto kind = session_.confirmation();
    if (kind == farm::Confirmation::Restart) {
        session_.confirmRestart();
        selection_ = {};
        selectedCrop_ = farm::CropId::Tomato;
        welcome_.clear();
        message_ = "New farm started; autosave will save this reset.";
    } else if (kind == farm::Confirmation::Save) {
        const bool saved=saveCurrent();
        session_.cancelConfirmation();
        message_=saved ? "Farm saved." : saveStatus_;
    } else if (kind == farm::Confirmation::Load) {
        farm::World loaded;
        std::string error;
        farm::OfflineSummary summary;
        if (farm::loadProgress(farm::SaveStore::defaultPath(), loaded, wallTime(), summary, error)) {
            automaticSaving_=true; autosaveElapsed_=0;
            welcome_=welcomeText(summary);
            session_.applyLoadedWorld(loaded);
            selection_ = {};
            selectedCrop_ = farm::CropId::Tomato;
            message_ = "Farm loaded. Space resumes play.";
        } else {
            session_.cancelConfirmation();
            message_ = error;
        }
    }
    focusedLastFrame_ = false;
    lastFrameTime_=GetTime();
    workerTime_=0;
}

void Game::handleActions() {
    if(!session_.confirming() && (IsKeyPressed(KEY_F2) || (!helpOpen_ && clicked(settingsButton())))) {
        settingsOpen_=!settingsOpen_; helpOpen_=false; return;
    }
    if(settingsOpen_) {
        if(IsKeyPressed(KEY_Q) || clicked(settingRow(0))) audio_.muted=!audio_.muted;
        if(IsKeyPressed(KEY_LEFT)) audio_.musicVolume=std::max(0.f,audio_.musicVolume-.25f);
        if(IsKeyPressed(KEY_RIGHT)) audio_.musicVolume=std::min(1.f,audio_.musicVolume+.25f);
        if(IsKeyPressed(KEY_DOWN)) audio_.effectsVolume=std::max(0.f,audio_.effectsVolume-.25f);
        if(IsKeyPressed(KEY_UP)) audio_.effectsVolume=std::min(1.f,audio_.effectsVolume+.25f);
        if(clicked(settingRow(1))) audio_.musicVolume=std::clamp((GetMousePosition().x-settingRow(1).x)/296,0.f,1.f);
        if(clicked(settingRow(2))) { audio_.effectsVolume=std::clamp((GetMousePosition().x-settingRow(2).x)/296,0.f,1.f); audio_.action(true); }
        if(IsKeyPressed(KEY_V) || clicked(settingRow(3))) reducedMotion_=!reducedMotion_;
        return;
    }
    if(!session_.confirming() && (IsKeyPressed(KEY_F1) || clicked(helpButton()))) {
        helpOpen_=!helpOpen_; return;
    }
    if(helpOpen_) return;
    if (session_.confirming()) {
        if (IsKeyPressed(KEY_Y) || clicked(dialogButton(true))) confirmAction();
        else if (IsKeyPressed(KEY_N) || clicked(dialogButton(false))) {
            session_.cancelConfirmation();
            focusedLastFrame_ = false;
            message_ = "Action cancelled.";
        }
        return;
    }
    const bool saveRequested = IsKeyPressed(KEY_F5) || (session_.paused() && clicked(dialogButton(true)));
    const bool loadRequested = IsKeyPressed(KEY_F9) || (session_.paused() && clicked(dialogButton(false)));
    if (saveRequested || loadRequested) {
#ifdef __EMSCRIPTEN__
        message_ = "Save/load is desktop-only for now.";
#else
        if (farm::SaveStore::defaultPath().empty()) message_ = "Save location is unavailable.";
        else if (saveRequested) session_.requestSave();
        else session_.requestLoad();
#endif
            return;
    }
    if (IsKeyPressed(KEY_R)) {
        session_.requestRestart();
            return;
    }
    if (IsKeyPressed(KEY_SPACE) || clicked(pauseButton())) {
        session_.togglePause();
        if(!session_.paused()) welcome_.clear();
            focusedLastFrame_ = false;
        return;
    }
    if (!session_.canAct()) return;
    const auto pressed = [](int key) { return IsKeyPressed(key) || IsKeyPressedRepeat(key); };
    const int horizontal = static_cast<int>(pressed(KEY_RIGHT)) - static_cast<int>(pressed(KEY_LEFT));
    const int vertical = static_cast<int>(pressed(KEY_DOWN)) - static_cast<int>(pressed(KEY_UP));
    if (horizontal || vertical) {
        selection_.move(horizontal, vertical);
        message_ = "Tile selected.";
    }
    const int keys[] = {KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE};
    for (int i = 0; i < static_cast<int>(farm::crops.size()); ++i) {
        if (IsKeyPressed(keys[i])) {
            selectedCrop_ = static_cast<farm::CropId>(i);
            message_ = "Seed selected; planted crops stay unchanged.";
        }
    }
    int action = -1;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const auto mouse = GetMousePosition();
        const auto layout = layoutForWindow();
        if (CheckCollisionPointRec(mouse, {layout.left, layout.top,
            layout.tile * farm::World::columns, layout.tile * farm::World::rows})) {
            selection_.column = static_cast<int>((mouse.x - layout.left) / layout.tile);
            selection_.row = static_cast<int>((mouse.y - layout.top) / layout.tile);
            message_ = "Tile selected.";
        }
        for (int i = 0; i < static_cast<int>(farm::crops.size()); ++i) {
            if (CheckCollisionPointRec(mouse, cropButton(i))) {
                selectedCrop_ = static_cast<farm::CropId>(i);
                message_ = "Seed selected; planted crops stay unchanged.";
            }
        }
        for (int i = 0; i < 6; ++i) if (CheckCollisionPointRec(mouse, actionButton(i))) action = i;
    }
    if (IsKeyPressed(KEY_B)) action = 0;
    else if (IsKeyPressed(KEY_P)) action = 1;
    else if (IsKeyPressed(KEY_H)) action = 2;
    else if (IsKeyPressed(KEY_M)) action = 3;
    else if (IsKeyPressed(KEY_U)) action = 4;
    else if (IsKeyPressed(KEY_T)) action = 5;
    if(action>=0) {
        const auto result=performAction(session_.world(),action,selection_,selectedCrop_);
        feedbackSuccess_=result==farm::Result::Success;
        const char* labels[]={"Land expanded","Plot planted","Harvest collected","Worker hired","Yield improved","Crop tended"};
        message_=feedbackSuccess_?labels[action]:actionReason(result,action);
        feedbackLabel_=feedbackSuccess_?labels[action]:"Unavailable";
        feedbackTile_=selection_; feedbackTime_=1.6;
        audio_.action(feedbackSuccess_);
    }
}

void Game::frame() {
#ifdef __EMSCRIPTEN__
    const int width = EM_ASM_INT({ return Math.max(360, Math.floor(document.getElementById('game').clientWidth)); });
    const int height = EM_ASM_INT({ return Math.max(520, Math.floor(document.getElementById('game').clientHeight)); });
    if (width != GetScreenWidth() || height != GetScreenHeight()) SetWindowSize(width, height);
    const bool focused = EM_ASM_INT({ return !document.hidden && document.activeElement === Module.canvas; }) != 0;
#else
    const bool focused = IsWindowFocused();
#endif
    const double now = GetTime();
    const double dt = lastFrameTime_ < 0 ? 0 : std::max(0.0, now - lastFrameTime_);
    lastFrameTime_ = now;
    update(dt,focused);
    render();
}

void Game::update(double dt,bool focused) {
    if(!preview_ && focused && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || GetKeyPressed()!=0)) audio_.start();
    audio_.update(focused);
    feedbackTime_=std::max(0.0,feedbackTime_-dt);
    playerWalking_=false;
    if (session_.canAct() && !helpOpen_ && !settingsOpen_ && !preview_) {
        session_.world().tick(dt);
        if(!reducedMotion_) workerTime_ += dt;
        playerWalking_=focused && (IsKeyDown(KEY_W)||IsKeyDown(KEY_A)||IsKeyDown(KEY_S)||IsKeyDown(KEY_D));
        if (focused && focusedLastFrame_) session_.world().move({
            static_cast<double>(IsKeyDown(KEY_D) - IsKeyDown(KEY_A)),
            static_cast<double>(IsKeyDown(KEY_S) - IsKeyDown(KEY_W))}, dt);
    }
    if (focused && !preview_) handleActions();
    if(automaticSaving_) {
        autosaveElapsed_+=dt;
        if(autosaveElapsed_>=30 && !session_.confirming()) { autosaveElapsed_=0; saveCurrent(); }
    }
    focusedLastFrame_ = focused;
}

void Game::render() const {
    const auto layout = layoutForWindow();
    const float tile = layout.tile;
    const float left = layout.left;
    const float top = layout.top;
    BeginDrawing();
    ClearBackground(Color{28,44,38,255});
    DrawRectangleGradientV(0,0,GetScreenWidth(),GetScreenHeight(),Color{42,65,51,255},Color{24,38,35,255});
    if(layout.wide) DrawRectangleRounded({6,64,312,GetScreenHeight()-150.0f},.06f,8,Color{33,48,41,255});
    DrawRectangleRounded({left-5,top-5,tile*10+8,tile*7+8},.035f,8,Color{164,137,91,255});
    DrawRectangleRounded({left-2,top-2,tile*10+2,tile*7+2},.035f,8,Color{44,65,45,255});
    const auto& world=session_.world();
    fitted("GRID-BASED-FARMING GAME",12,12,helpButton().x-24,20,art::cream);
    fitted("$"+farm::formatMoney(world.balance())+"   +$"+farm::formatMoney(world.incomeRate())+"/s",12,38,GetScreenWidth()-24,14,GOLD);
    DrawRectangleRec(helpButton(),Color{46,62,49,255});
    DrawText("F1 Help",GetScreenWidth()-170,24,12,RAYWHITE);
    DrawRectangleRec(pauseButton(),Color{65,88,65,255});
    DrawText(session_.paused()?"Resume":"Pause",GetScreenWidth()-90,24,12,RAYWHITE);
    const char* shortNames[]={"Tomato","Berry","Carrot","Beet","Strawb"};
    for(int i=0;i<5;++i) {
        auto button=cropButton(i); bool active=static_cast<int>(selectedCrop_)==i;
        DrawRectangleRounded(button,.2f,6,active?Color{105,88,52,255}:Color{48,68,54,255});
        if(active) DrawRectangleLinesEx(button,2,GOLD);
        const auto id=static_cast<farm::CropId>(i);
        fitted(std::to_string(i+1)+" "+(layout.wide?farm::crops[i].name:shortNames[i]),button.x+4,button.y+5,button.width-8,12,RAYWHITE);
        if(world.automated(id)) DrawCircle(static_cast<int>(button.x+button.width-7),static_cast<int>(button.y+25),3,cropColor(id));
    }
    const float detailY=layout.detailTop();
    const auto* crop=farm::cropDefinition(selectedCrop_);
    const auto* selected=world.tile(selection_.column,selection_.row);
    fitted(std::string(crop->name)+" | "+std::to_string(world.plotCount(selectedCrop_))+" plots | Lv "+std::to_string(world.upgradeLevel(selectedCrop_)),12,detailY,layout.panelWidth,14,GOLD);
    fitted("Yield $"+farm::formatMoney(world.yield(selectedCrop_))+" / "+std::to_string(static_cast<int>(crop->cycleSeconds))+"s | Worker "+(world.automated(selectedCrop_)?"hired":"needed"),12,detailY+20,layout.panelWidth,12,LIGHTGRAY);
    const std::string tileText="Tile "+std::to_string(selection_.column+1)+","+std::to_string(selection_.row+1)+": "+(selected->planted?std::string(farm::cropDefinition(selected->crop)->name)+(selected->age>=farm::cropDefinition(selected->crop)->cycleSeconds?" READY":" growing"):(selected->owned?"empty soil":"unowned"));
    fitted(tileText,12,detailY+40,layout.panelWidth,12,RAYWHITE);
    if(layout.wide && layout.height>=640) {
        DrawText("WORKER ASSIGNMENT",12,278,12,GOLD);
        fitted(world.automated(selectedCrop_)?"Worker "+std::to_string(static_cast<int>(selectedCrop_)+1)+" visits all "+std::string(crop->name):"Hire a worker to automate this crop.",12,298,layout.panelWidth,12,LIGHTGRAY);
        fitted("Worker colour and number match the crop.",12,316,layout.panelWidth,12,LIGHTGRAY);
    }
    const std::string labels[]={"B Land $"+farm::formatMoney(world.landPrice()),"P Plot $"+farm::formatMoney(world.plotPrice(selectedCrop_)),"H Harvest",world.automated(selectedCrop_)?"M Hired":"M Hire $"+farm::formatMoney(world.workerPrice(selectedCrop_)),"U Yield $"+farm::formatMoney(world.upgradePrice(selectedCrop_)),world.tendCooldown()>0?"T Wait "+std::to_string(static_cast<int>(std::ceil(world.tendCooldown())))+"s":"T Tend +10%"};
    std::string hint=message_;
    for(int i=0;i<6;++i) {
        auto button=actionButton(i);const auto available=actionAvailability(world,i,selection_,selectedCrop_);
        const bool enabled=available==farm::Result::Success;
        DrawRectangleRounded({button.x,button.y+2,button.width,button.height},.18f,6,Color{21,34,29,255});
        DrawRectangleRounded(button,.18f,6,enabled?Color{70,107,76,255}:Color{44,53,46,255});
        fitted(labels[i],button.x+5,button.y+10,button.width-10,12,enabled?RAYWHITE:GRAY);
        if(CheckCollisionPointRec(GetMousePosition(),button)) {
            DrawRectangleLinesEx(button,1,enabled?GOLD:GRAY);
            hint=enabled?"Ready: "+labels[i]:actionReason(available,i);
        }
    }
    fitted(hint,12,layout.hintTop(),layout.panelWidth,11,GOLD);
    if(layout.wide) {
        DrawText("FARM",static_cast<int>(left),70,14,LIGHTGRAY);
        if(layout.height>=640) {
            DrawText("Arrows / click select tiles",12,500,12,LIGHTGRAY);
            DrawText("1-5 select crops | F1 controls",12,520,12,LIGHTGRAY);
        }
    }
    for (int row = 0; row < farm::World::rows; ++row) {
        for (int col = 0; col < farm::World::columns; ++col) {
            Rectangle cell{left + col * tile, top + row * tile, tile - 2, tile - 2};
            const auto* ground = session_.world().tile(col, row);
            art::plot(cell,*ground,col,row,reducedMotion_?0:workerTime_);
            if (ground->planted && ground->age >= farm::cropDefinition(ground->crop)->cycleSeconds)
                DrawRectangleLinesEx({cell.x + 3, cell.y + 3, tile - 8, tile - 8}, 1, RAYWHITE);
            if(ground->planted && ground->crop==selectedCrop_ && world.automated(selectedCrop_))
                DrawRectangleLinesEx({cell.x+1,cell.y+1,cell.width-2,cell.height-2},1,cropColor(selectedCrop_));
            if (col == selection_.column && row == selection_.row) DrawRectangleLinesEx(cell, 2, GOLD);
        }
    }
    for(int i=0;i<5;++i) {
        const auto crop=static_cast<farm::CropId>(i);
        if(!session_.world().automated(crop)) continue;
        const auto worker=workerPosition(session_.world(),crop,workerTime_);
        const float x=left+static_cast<float>(worker.x)*tile, y=top+static_cast<float>(worker.y)*tile;
        const float size=std::max(3.0f,tile*.21f);
        art::person(x,y,size,art::cropColor(crop),workerTime_+i,!reducedMotion_);
        DrawText(TextFormat("%d",i+1),static_cast<int>(x+size),static_cast<int>(y-size),10,WHITE);
    }
    if(layout.wide && top+tile*7+100<GetScreenHeight()-80) {
        const float y=top+tile*7+28;
        DrawText("A LITTLE CARE. A LITTLE GROWTH.",static_cast<int>(left),static_cast<int>(y),14,art::cream);
        const char* stages[]={"Sprout","Growing","Harvest ready"};
        const double progress[]={.1,.5,1.0};
        for(int i=0;i<3;++i) {
            const float x=left+i*std::min(170.f,tile*10/3);
            art::plant(x+16,y+49,26,selectedCrop_,progress[i],0);
            DrawText(stages[i],static_cast<int>(x+38),static_cast<int>(y+44),12,LIGHTGRAY);
        }
    }
    const auto player = session_.world().player();
    art::person(left+static_cast<float>(player.x)*tile,top+static_cast<float>(player.y)*tile,tile*.23f,Color{235,220,169,255},workerTime_,playerWalking_ && !reducedMotion_);
    if(feedbackTime_>0) {
        const float x=left+feedbackTile_.column*tile,y=top+feedbackTile_.row*tile;
        DrawRectangleLinesEx({x,y,tile-2,tile-2},3,feedbackSuccess_?art::cream:Color{234,133,112,255});
        const float labelY=y-(reducedMotion_?0:static_cast<float>((1.6-feedbackTime_)*10));
        const float labelX=std::clamp(x,12.f,std::max(12.f,GetScreenWidth()-150.f));
        DrawRectangleRounded({labelX,labelY,138,22},.2f,4,Color{28,44,38,245});
        fitted(feedbackLabel_,labelX+5,labelY+5,128,12,art::cream);
    }
    const int goalY = GetScreenHeight() - 64;
    DrawText("NEXT GOAL",16,goalY,12,GOLD);
    const auto goal = session_.world().nextGoal();
    const float goalSize = std::min(10.0f, 10.0f * (GetScreenWidth()-154) / std::max(1, MeasureText(goal.c_str(),10)));
    DrawTextEx(GetFontDefault(),goal.c_str(),{16,static_cast<float>(goalY+18)},goalSize,1,RAYWHITE);
    fitted("Space pauses | F1 help",16,goalY+36,GetScreenWidth()-156,10,LIGHTGRAY);
    DrawRectangleRounded(settingsButton(),.2f,6,Color{65,88,65,255});
    DrawText("F2 Settings",GetScreenWidth()-116,GetScreenHeight()-40,12,art::cream);
    if (!helpOpen_ && !settingsOpen_ && (session_.paused() || session_.confirming())) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.75f));
        const int x = GetScreenWidth() / 2 - 148;
        const int y = GetScreenHeight() / 2;
        if (session_.confirming()) {
            const auto kind = session_.confirmation();
            const char* title = kind == farm::Confirmation::Restart ? "Restart this farm?" :
                kind == farm::Confirmation::Save ? "Save this farm?" : "Load saved farm?";
            const char* detail = kind == farm::Confirmation::Restart ? "All current progress will be lost." :
                kind == farm::Confirmation::Save ? "Replaces the previous save, if any." : "Replaces this farm; loads paused.";
            const char* action = kind == farm::Confirmation::Restart ? "Restart" :
                kind == farm::Confirmation::Save ? "Save" : "Load";
            DrawText(title, x, y - 65, 22, GOLD);
            fitted(detail,x,y-30,296,12,RAYWHITE);
            DrawText("Y: confirm   N: cancel", x, y - 5, 14, LIGHTGRAY);
            for (bool confirm : {true, false}) {
                const auto button = dialogButton(confirm);
                DrawRectangleRec(button, Color{65, 88, 65, 255});
                DrawText(confirm ? action : "Cancel", static_cast<int>(button.x + 12),
                    static_cast<int>(button.y + 8), 16, RAYWHITE);
            }
        } else {
            DrawText("Paused", x, y - 65, 26, GOLD);
            DrawText("Space or Resume to continue | R restart", x, y - 28, 12, RAYWHITE);
            DrawText("F5 save | F9 load", x, y - 5, 14, LIGHTGRAY);
            for (bool save : {true, false}) {
                const auto button = dialogButton(save);
                DrawRectangleRec(button, Color{65, 88, 65, 255});
                DrawText(save ? "Save (F5)" : "Load (F9)", static_cast<int>(button.x + 10),
                    static_cast<int>(button.y + 8), 16, RAYWHITE);
            }
            DrawText(message_.c_str(), x, y + 80, 10, GOLD);
            const float welcomeSize=std::min(10.0f,10.0f*296/std::max(1,MeasureText(welcome_.c_str(),10)));
            DrawTextEx(GetFontDefault(),welcome_.c_str(),{static_cast<float>(x),static_cast<float>(y+98)},welcomeSize,1,RAYWHITE);
            DrawRectangleRec(pauseButton(), Color{65, 88, 65, 255});
            DrawText("Resume", GetScreenWidth() - 90, 24, 12, RAYWHITE);
        }
    }
    if(helpOpen_) {
        DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,.94f));
        const int x=20, y=60;
        DrawText("CONTROLS & WORKERS",x,y,20,GOLD);
        const char* lines[]={"WASD: move farmer (cosmetic)","Arrows / click: select a tile","1-5: select crop for plots / upgrades","B: buy land   P: establish plot","H: harvest selected ripe plot","M: hire worker for selected crop","U: upgrade selected crop yield","T: tend selected tile (+10% cycle)","Space: pause   R: confirm restart","F5: save   F9: load (desktop)","Workers 1-5 match the crop buttons.","Each visits all plots of its crop.","Production is independent of walking.","F1 or Help: close this panel"};
        for(int i=0;i<14;++i) fitted(lines[i],x,y+38+i*24,GetScreenWidth()-40,14,RAYWHITE);
        DrawRectangleRec(helpButton(),Color{65,88,65,255});
        DrawText("Close F1",GetScreenWidth()-170,24,12,RAYWHITE);
    }
    if(settingsOpen_) {
        DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,.88f));
        const float x=settingRow(0).x,y=settingRow(0).y;
        DrawText("SOUND & COMFORT",static_cast<int>(x),static_cast<int>(y-52),22,art::gold);
        const std::string labels[]={audio_.muted?"Q  Sound muted":"Q  Sound on","Music  "+std::to_string(static_cast<int>(audio_.musicVolume*100))+"%   Left / Right","Effects  "+std::to_string(static_cast<int>(audio_.effectsVolume*100))+"%   Down / Up",reducedMotion_?"V  Reduced motion: on":"V  Reduced motion: off"};
        for(int i=0;i<4;++i) {
            auto r=settingRow(i);
            DrawRectangleRounded(r,.2f,6,Color{58,79,61,255});
            if(i==1 || i==2) DrawRectangleRec({r.x,r.y+r.height-4,r.width*(i==1?audio_.musicVolume:audio_.effectsVolume),4},art::gold);
            fitted(labels[i],r.x+10,r.y+10,r.width-20,14,art::cream);
        }
        fitted(audio_.available()?"Click a volume bar to set its level.":"Audio unavailable until first interaction/device ready.",x,y+210,296,12,LIGHTGRAY);
        fitted("Settings apply to this session. F2 closes.",x,y+232,296,12,LIGHTGRAY);
        DrawRectangleRounded(settingsButton(),.2f,6,Color{65,88,65,255});
        DrawText("F2 Close",GetScreenWidth()-116,GetScreenHeight()-40,12,art::cream);
    }
    if(!helpOpen_) {
        DrawRectangleRounded(settingsButton(),.2f,6,Color{65,88,65,255});
        DrawText(settingsOpen_?"F2 Close":"F2 Settings",GetScreenWidth()-116,GetScreenHeight()-40,12,art::cream);
    }
    const float statusSize=std::min(10.0f,10.0f*(GetScreenWidth()-32)/std::max(1,MeasureText(saveStatus_.c_str(),10)));
    DrawTextEx(GetFontDefault(),saveStatus_.c_str(),{16,static_cast<float>(GetScreenHeight()-12)},statusSize,1,LIGHTGRAY);
    EndDrawing();
}
