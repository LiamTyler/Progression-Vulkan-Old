#include "audio/audio_system.hpp"

namespace Progression {

    ALCdevice* AudioSystem::device_ = NULL;
    ALCcontext* AudioSystem::context_ = NULL;
    std::unordered_map<std::string, AudioFile*> AudioSystem::audioFiles_ = {};
    std::unordered_map<std::string, AudioSource*> AudioSystem::audioSources_ = {};
    
    void AudioSystem::Init(const config::Config& config) {
        device_ = alcOpenDevice(NULL);
        if (!device_) {
            std::cout << "Could not open audio device" << std::endl;
            return;
        }
        context_ = alcCreateContext(device_, NULL);
        alcMakeContextCurrent(context_);
    }

    void AudioSystem::Free() {
        for (auto& audioFile : audioFiles_)
            delete audioFile.second;
        for (auto& audioSource: audioSources_)
            delete audioSource.second;
        if (context_) {
            alcMakeContextCurrent(NULL);
            alcDestroyContext(context_);
            alcCloseDevice(device_);
        }
    }

    void AudioSystem::PlayAllAudio() {
        for (auto& source : audioSources_)
            source.second->Play();
    }

    void AudioSystem::PauseAllAudio() {
        for (auto& source : audioSources_)
            source.second->Pause();
    }

    void AudioSystem::StopAllAudio() {
        for (auto& source : audioSources_)
            source.second->Stop();
    }

    std::vector<std::string> AudioSystem::getPlaybackDevices() {
        std::vector<std::string> devices;
        const ALCchar* device = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
        while (*device) {
            std::string dev(device);
            device += dev.length() + 1;
            devices.push_back(dev);
        }
        return devices;
    }

    std::vector<std::string> AudioSystem::getCaptureDevices() {
        std::vector<std::string> devices;
        const ALCchar* device = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
        while (*device) {
            std::string dev(device);
            device += dev.length() + 1;
            devices.push_back(dev);
        }
        return devices;
    }

} // namespace Progression
