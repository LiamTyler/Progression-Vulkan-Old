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

            static bool LoadAudioFile(const std::string& path);
            static bool FreeAudioFile(const std::string& path);

            static void PlayAllAudio();
            static void PauseAllAudio();
            static void StopAllAudio();

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
