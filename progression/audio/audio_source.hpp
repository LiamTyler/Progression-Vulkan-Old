#pragma once

#include <AL/al.h>
#include <AL/alc.h>
#include "core/common.h"
#include "audio/audio_file.hpp"

namespace Progression {

    class AudioSource {
        public:
            AudioSource(const glm::vec3& pos = glm::vec3(0));
            ~AudioSource();
            AudioSource(const AudioSource&) = delete;
            AudioSource& operator=(const AudioSource&) = delete;

            void Play();
            void Pause();
            void Stop();
            void setAudio(AudioFile* file);
            void setVolume(float volume);
            void setLooping(bool b);
            void setPosition(const glm::vec3& pos);
            void setVelocity(const glm::vec3& vel);
            void setDirection(const glm::vec3& forwardDir);

            AudioFile* getAudio() const { return audioFile_; }
            float getVolume() const;
            bool isLooping() const;
            glm::vec3 getPosition() const;
            glm::vec3 getVelocity() const;
            glm::vec3 getDirection() const;

        private:
            glm::vec3 position_;
            ALuint source_;
            AudioFile* audioFile_;
    };

} // namespace Progression
