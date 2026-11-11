#include "app/Game.h"
#include "app/UiLayout.h"
#include "core/SaveCodec.h"
#include "storage/ProgressStore.h"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>

struct AudioRuntimeChecks {
    void run() {
        FarmAudio audio;
        audio.start();
        const auto check=[](bool ok,const char* label) { if(!ok) throw std::runtime_error(label); };
        check(audio.available(),"audio device and melody available");
        audio.update(true); check(IsSoundPlaying(audio.music_),"music starts");
        audio.action(true); check(IsSoundPlaying(audio.good_),"success tone starts");
        audio.update(false);
        check(!IsSoundPlaying(audio.music_) && !IsSoundPlaying(audio.good_),"focus loss silences audio");
        audio.update(true); check(IsSoundPlaying(audio.music_),"focus return resumes music");
        audio.musicVolume=0; audio.update(true);
        check(!IsSoundPlaying(audio.music_),"zero music volume pauses playback");
        audio.musicVolume=.05f; audio.update(true);
        audio.muted=true; audio.update(true); audio.action(false);
        check(!IsSoundPlaying(audio.music_) && !IsSoundPlaying(audio.bad_),"muted action stays silent");
        audio.muted=false; audio.update(true);
        audio.effectsVolume=0; audio.action(false);
        check(!IsSoundPlaying(audio.bad_),"zero effects volume stays silent");
        audio.effectsVolume=.05f; audio.action(false);
        check(IsSoundPlaying(audio.bad_),"failure tone starts");
        // wait for two real playback loops
        const auto start=std::chrono::steady_clock::now();
        int loops=0;
        while(loops<2 && std::chrono::steady_clock::now()-start<std::chrono::seconds(38)) {
            if(!IsSoundPlaying(audio.music_)) {
                ++loops; audio.update(true);
                check(IsSoundPlaying(audio.music_),"melody restarts after completion");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        check(loops==2,"two complete melody loops");
        std::cout<<"PASS: audio controls and two real 16-second playback loops\n";
    }
};

// event IDs come from raylib 6.0's rcore.c
// dispatch input directly so the checks don't need to steal window focus
struct GameRuntimeChecks {
    int checks=0;
    void check(bool ok,const char* label) {
        if(!ok) throw std::runtime_error(label);
        ++checks;
    }
    void key(Game& game,int key) {
        PlayAutomationEvent({0,2,{key,0,0,0}}); // INPUT_KEY_DOWN
        game.handleActions();
        PlayAutomationEvent({0,1,{key,0,0,0}}); // INPUT_KEY_UP
        PollInputEvents();
    }
    void click(Game& game,float x,float y) {
        PlayAutomationEvent({0,7,{static_cast<int>(x),static_cast<int>(y),0,0}});
        PlayAutomationEvent({0,6,{MOUSE_BUTTON_LEFT,0,0,0}});
        game.handleActions();
        PlayAutomationEvent({0,5,{MOUSE_BUTTON_LEFT,0,0,0}});
        PollInputEvents();
    }
    void advance(Game& game,double seconds) {
        game.update(seconds,false);
        game.render();
    }
    static std::string farm(const Game& game) { return farm::SaveCodec::encode(game.session_.world()); }
    void run(const std::filesystem::path& output) {
        Game game;
        check(game.automaticSaving_ && !game.session_.paused(),"fresh isolated farm starts running");
        key(game,KEY_P);
        check(game.session_.world().tile(0,0)->planted && game.feedbackSuccess_,"plant input and success feedback");
        key(game,KEY_H);
        check(!game.feedbackSuccess_ && game.message_=="Select a ripe plot to harvest.","failed harvest gives reason");
        advance(game,10);
        key(game,KEY_H);
        check(game.feedbackSuccess_ && game.session_.world().balance()==1035,"mature manual harvest");
        key(game,KEY_M);
        check(game.session_.world().automated(farm::CropId::Tomato),"worker hire input");

        key(game,KEY_F2);
        check(game.settingsOpen_,"settings opens");
        auto saved=farm(game);
        advance(game,2);
        check(farm(game)==saved,"settings freezes production");
        key(game,KEY_Q); check(game.audio_.muted,"keyboard mute");
        key(game,KEY_Q); check(!game.audio_.muted,"keyboard unmute");
        for(int i=0;i<6;++i) key(game,KEY_LEFT);
        check(game.audio_.musicVolume==0,"music lower bound");
        for(int i=0;i<6;++i) key(game,KEY_RIGHT);
        check(game.audio_.musicVolume==1,"music upper bound");
        for(int i=0;i<6;++i) key(game,KEY_DOWN);
        check(game.audio_.effectsVolume==0,"effects lower bound");
        for(int i=0;i<6;++i) key(game,KEY_UP);
        check(game.audio_.effectsVolume==1,"effects upper bound");
        const float x=GetScreenWidth()/2.f-148,y=GetScreenHeight()/2.f-100;
        click(game,x+74,y+68);
        check(game.audio_.musicVolume==.25f,"mouse music volume");
        click(game,x+148,y+118);
        check(game.audio_.effectsVolume==.5f,"mouse effects volume");
        click(game,x+30,y+18); check(game.audio_.muted,"mouse mute");
        click(game,x+30,y+168); check(game.reducedMotion_,"mouse reduced motion");
        key(game,KEY_F2);
        check(!game.settingsOpen_ && !game.session_.paused(),"settings restores running state");
        const double balance=game.session_.world().balance(), animation=game.workerTime_;
        advance(game,10);
        check(game.session_.world().balance()==balance+60,"reduced motion preserves worker earnings");
        check(game.workerTime_==animation,"reduced motion stops decorative clock");
        key(game,KEY_SPACE); key(game,KEY_F2); key(game,KEY_V); key(game,KEY_F2);
        check(game.session_.paused() && !game.reducedMotion_,"settings preserves paused state");
        key(game,KEY_F1);
        saved=farm(game); advance(game,1);
        check(farm(game)==saved,"help preserves paused farm");
        key(game,KEY_F1); key(game,KEY_SPACE);
        key(game,KEY_F1); saved=farm(game); advance(game,1);
        check(farm(game)==saved,"help freezes running farm");
        key(game,KEY_F2);
        check(game.settingsOpen_ && !game.helpOpen_,"help to settings stays paused");
        key(game,KEY_F2);

        // 600 simulated seconds with a few actions along the way
        for(int second=0;second<600;++second) {
            if(second==0) { key(game,KEY_RIGHT); key(game,KEY_B); key(game,KEY_THREE); key(game,KEY_P); key(game,KEY_M); }
            if(second==60) { key(game,KEY_U); check(game.feedbackSuccess_,"upgrade during playthrough"); }
            if(second%30==0) key(game,KEY_T);
            advance(game,1);
        }
        check(game.session_.world().balance()>balance,"scripted session earns income");
        check(game.session_.world().plotCount(farm::CropId::Carrot)==1,"second crop planted through input");
        check(game.session_.world().automated(farm::CropId::Carrot),"second worker hired through input");
        check(game.saveOnClose(),"isolated final save succeeds");
        farm::World loaded; std::string error;
        check(farm::SaveStore::load(farm::SaveStore::defaultPath(),loaded,error),"saved session decodes");
        check(farm::SaveCodec::encode(loaded)==farm(game),"saved farm matches live session");
        const auto slot=farm::SaveStore::defaultPath();
        farm::OfflineSummary summary;
        const auto now=loaded.savedAt()+28800;
        check(farm::loadProgress(slot,loaded,now,summary,error) && summary.credited==28800,"eight-hour return");
        const double returnedBalance=loaded.balance();
        check(farm::loadProgress(slot,loaded,now,summary,error) && summary.earnings==0 && loaded.balance()==returnedBalance,"repeat return cannot duplicate rewards");
        // capture the farm reached by the scripted session
        for(auto size:{std::pair<int,int>{360,520},{900,520},{1000,800}}) {
            SetWindowSize(size.first,size.second); PollInputEvents();
            for(int i=0;i<5;++i) game.render();
            const auto name=output/(std::to_string(size.first)+"x"+std::to_string(size.second)+".png");
            TakeScreenshot(std::filesystem::relative(name).string().c_str());
            check(std::filesystem::exists(name),"runtime screenshot written");
        }
        key(game,KEY_F2);
        check(game.settingsOpen_,"settings screenshot panel open");
        for(int i=0;i<5;++i) game.render();
        TakeScreenshot(std::filesystem::relative(output/"settings.png").string().c_str());
        check(std::filesystem::exists(output/"settings.png"),"settings screenshot written");
        std::cout<<"PASS: "<<checks<<" runtime checks; 600 simulated seconds; isolated save "<<slot<<'\n';
    }
};

int main() {
    const auto output=std::filesystem::absolute("runtime-check-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(output);
#ifdef _WIN32
    if(_wputenv_s(L"LOCALAPPDATA",output.c_str())!=0) return 2;
#else
    if(setenv("XDG_DATA_HOME",output.c_str(),1)!=0) return 2;
#endif
    if(farm::SaveStore::defaultPath()!=output/"Grid-Based-Farming-Game"/"idle.save") return 2;
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN|FLAG_WINDOW_RESIZABLE);
    InitWindow(1000,800,"Grid-Based-Farming Game runtime checks");
    if(!IsWindowReady()) return 2;
    int result=0;
    try { GameRuntimeChecks{}.run(output); AudioRuntimeChecks{}.run(); }
    catch(const std::exception& error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; result=1; }
    CloseWindow();
    return result;
}
