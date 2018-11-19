#include "audio/audio_source.hpp"

namespace Progression {

    AudioSource::AudioSource(const glm::vec3& pos) :
        position_(pos),
        source_(-1),
        audioFile_(nullptr)
    {
        alGenSources(1, &source_);
        setPosition(position_);
        setVelocity(glm::vec3(0));
        setDirection(glm::vec3(0, 0, 1));
    }


    AudioSource::~AudioSource() {
        if (source_ != -1)
            alDeleteSources(1, &source_);
    }

    void AudioSource::Play() {
        alSourcePlay(source_);
    }

    void AudioSource::Pause() {
        alSourcePause(source_);
    }

    void AudioSource::Stop() {
        alSourceStop(source_);
    }

    void AudioSource::setAudio(AudioFile* audioFile) {
        audioFile_ = audioFile;
        alSourcei(source_, AL_BUFFER, audioFile_->getBuffer());
    }

    void AudioSource::setVolume(float volume) {
        alSourcef(source_, AL_GAIN, std::fmax(0.0f, volume));
    }

    float AudioSource::getVolume() const {
        float vol;
        alGetSourcef(source_, AL_GAIN, &vol);
        return vol;
    }

    void AudioSource::setLooping(bool b) {
        alSourcei(source_, AL_LOOPING, b ? AL_TRUE : AL_FALSE);
    }

    bool AudioSource::isLooping() const {
        int looping;
        alGetSourcei(source_, AL_LOOPING, &looping);
        return looping;
    }

    void AudioSource::setPosition(const glm::vec3& pos) {
        position_ = pos;
        alSource3f(source_, AL_POSITION, pos[0], pos[1], pos[2]);
    }

    void AudioSource::setVelocity(const glm::vec3& vel) {
        alSource3f(source_, AL_VELOCITY, vel[0], vel[1], vel[2]);
    }

    void AudioSource::setDirection(const glm::vec3& dir) {
        alSource3f(source_, AL_DIRECTION, dir[0], dir[1], dir[2]);
    }

    glm::vec3 AudioSource::getPosition() const {
        return position_;
    }

    glm::vec3 AudioSource::getVelocity() const {
        glm::vec3 vel;
        alGetSourcefv(source_, AL_VELOCITY, &vel[0]);
        return vel;
    }

    glm::vec3 AudioSource::getDirection() const {
        glm::vec3 dir;
        alGetSourcefv(source_, AL_DIRECTION, &dir[0]);
        return dir;
    }

} // namespace Progression
