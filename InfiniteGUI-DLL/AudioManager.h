#pragma once
#include "miniaudio/miniaudio.h"
#include <string>
#include <map>

class AudioManager {
public:
    static AudioManager& Instance()
    {
        static AudioManager instance;
        return instance;
    }

    bool Init();
    void Shutdown();

    bool LoadSound(const std::string& path); // 加载音频
    void Play();                              // 播放
    void SetVolume(float volume);             // 0.0 ~ 1.0
    float GetVolume();
    void playSound(std::string soundName, float soundVolume);
private:
    AudioManager() = default;

    ma_engine engine;
    ma_sound sound;
    bool soundLoaded = false;
};