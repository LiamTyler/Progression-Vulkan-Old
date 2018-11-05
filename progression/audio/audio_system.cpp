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

    AudioFile* AudioSystem::LoadAudio(const std::string& path) {
        if (audioFiles_.find(path) != audioFiles_.end())
            return audioFiles_[path];

        AudioFile* aFile = new AudioFile;
        if (aFile->Open(path) && aFile->Read()) {
            audioFiles_[path] = aFile;
            return aFile;
        } else {
            delete aFile;
            return nullptr;
        }
    }

    AudioFile* AudioSystem::getAudio(const std::string& path) {
        if (audioFiles_.find(path) != audioFiles_.end())
            return audioFiles_[path];
        else
            return nullptr;
    }

    void AudioSystem::FreeAudio(const std::string& path) {
        if (audioFiles_.find(path) == audioFiles_.end())
            return;
        delete audioFiles_[path];
        audioFiles_.erase(path);
    }

    AudioSource* AudioSystem::AddSource(const std::string& sourceName) {
        if (audioSources_.find(sourceName) != audioSources_.end())
            return audioSources_[sourceName];
        audioSources_[sourceName] = new AudioSource;
        return audioSources_[sourceName];
    }

    AudioSource* AudioSystem::getSource(const std::string& sourceName) {
        if (audioSources_.find(sourceName) == audioSources_.end())
            return nullptr;
        return audioSources_[sourceName];
    }

    void AudioSystem::RemoveSource(const std::string& sourceName) {
        if (audioSources_.find(sourceName) == audioSources_.end())
            return;
        delete audioSources_[sourceName];
        audioSources_.erase(sourceName);
    }

    void AudioSystem::PlayAll() {
        for (auto& source : audioSources_)
            source.second->Play();
    }

    void AudioSystem::PauseAll() {
        for (auto& source : audioSources_)
            source.second->Pause();
    }

    void AudioSystem::StopAll() {
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
