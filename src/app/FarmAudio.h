#pragma once
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <vector>

// synthesize the notes locally, without audio files
class FarmAudio {
public:
    FarmAudio() = default;
    FarmAudio(const FarmAudio&) = delete;
    FarmAudio& operator=(const FarmAudio&) = delete;
    float musicVolume=.25f, effectsVolume=.5f;
    bool muted=false;
    ~FarmAudio() {
        if(!started_) return;
        if(IsSoundValid(music_)) UnloadSound(music_);
        if(IsSoundValid(good_)) UnloadSound(good_);
        if(IsSoundValid(bad_)) UnloadSound(bad_);
        if(IsAudioDeviceReady()) CloseAudioDevice();
    }
    void start() {
        if(started_) return;
        started_=true;
        InitAudioDevice();
        if(!IsAudioDeviceReady()) return;
        music_=synthesize(true,false); good_=synthesize(false,false); bad_=synthesize(false,true);
    }
    bool available() const { return IsAudioDeviceReady() && IsSoundValid(music_); }
    void update(bool focused) {
        if(!available()) return;
        SetSoundVolume(music_,muted?0:musicVolume);
        SetSoundVolume(good_,muted?0:effectsVolume);
        SetSoundVolume(bad_,muted?0:effectsVolume);
        if(!focused || muted || musicVolume==0) {
            if(IsSoundPlaying(music_)) { PauseSound(music_); paused_=true; }
        } else if(!IsSoundPlaying(music_)) {
            if(paused_) { ResumeSound(music_); paused_=false; }
            else PlaySound(music_);
        }
        if(!focused || muted) { StopSound(good_); StopSound(bad_); }
    }
    void action(bool success) {
        if(!available() || muted || effectsVolume==0) return;
        auto sound=success?good_:bad_;
        SetSoundVolume(sound,effectsVolume);
        PlaySound(sound);
    }
private:
#ifdef GRIDFARM_RUNTIME_CHECKS
    friend struct AudioRuntimeChecks;
#endif
    bool started_=false;
    bool paused_=false;
    Sound music_{},good_{},bad_{};
    static Sound synthesize(bool music,bool error) {
        constexpr int rate=22050;
        const float duration=music?16.0f:.24f;
        std::vector<float> samples(static_cast<std::size_t>(rate*duration));
        constexpr float notes[]={261.63f,329.63f,392.f,329.63f,293.66f,349.23f,440.f,349.23f,261.63f,329.63f,392.f,523.25f,293.66f,349.23f,392.f,329.63f};
        for(std::size_t i=0;i<samples.size();++i) {
            const float t=static_cast<float>(i)/rate;
            const float local=music?std::fmod(t,1.0f):t;
            const float f=music?notes[static_cast<int>(t)%16]:(error?180.f:660.f+(t>.1f?220.f:0));
            const float envelope=std::min(1.f,local/.015f)*std::exp(-local*(music?4.f:15.f));
            const float fade=std::min(1.f,(duration-t)/.03f);
            samples[i]=.24f*envelope*fade*(std::sin(2*PI*f*t)+.25f*std::sin(4*PI*f*t));
        }
        Wave wave{static_cast<unsigned int>(samples.size()),rate,32,1,samples.data()};
        return LoadSoundFromWave(wave);
    }
};
