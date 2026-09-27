#include "AudioManager.h"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"
#include "FileUtils.h"

bool AudioManager::Init()
{
    if (ma_engine_init(NULL, &engine) != MA_SUCCESS)
        return false;

    return true;
}

void AudioManager::Shutdown()
{
    if (soundLoaded)
        ma_sound_uninit(&sound);

    ma_engine_uninit(&engine);
}

bool AudioManager::LoadSound(const std::string& path)
{
    if (soundLoaded)
        ma_sound_uninit(&sound);

    if (ma_sound_init_from_file(&engine, path.c_str(),
        MA_SOUND_FLAG_STREAM, NULL, NULL, &sound) != MA_SUCCESS)
    {
        soundLoaded = false;
        return false;
    }

    soundLoaded = true;
    return true;
}

void AudioManager::Play()
{
    if (soundLoaded)
        ma_sound_start(&sound);
}

void AudioManager::SetVolume(float volume)
{
    if (soundLoaded)
        ma_sound_set_volume(&sound, volume);
}

float AudioManager::GetVolume()
{
    if (!soundLoaded) return 0.0f;
    return ma_sound_get_volume(&sound);
}


void AudioManager::playSound(std::string soundName, float soundVolume)
{
    AudioManager::Instance().LoadSound(FileUtils::GetSoundPath(soundName));
    AudioManager::Instance().SetVolume(soundVolume);
    AudioManager::Instance().Play();
}