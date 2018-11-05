#pragma once

#include "core/config.h"
#include "audio/audio_file.hpp"
#include "audio/audio_source.hpp"

namespace Progression {

    class AudioSystem {
        public:
            AudioSystem() = delete;
            ~AudioSystem() = delete;

            static void Init(const config::Config& config);
            static void Free();

            static AudioFile* LoadAudio(const std::string& path);
            static AudioFile* getAudio(const std::string& path);
            static void FreeAudio(const std::string& path);

            static AudioSource* AddSource(const std::string& sourceName);
            static AudioSource* getSource(const std::string& sourceName);
            static void RemoveSource(const std::string& sourceName);


            static void PlayAll();
            static void PauseAll();
            static void StopAll();

            static std::vector<std::string> getPlaybackDevices();
            static std::vector<std::string> getCaptureDevices();
            // TODO: create ability to change audio devices
            
        private:
            static std::unordered_map<std::string, AudioFile*> audioFiles_;
            static std::unordered_map<std::string, AudioSource*> audioSources_;

            static ALCdevice* device_;
            static ALCcontext* context_;
    };

} // namespace Progression
