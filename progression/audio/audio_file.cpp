#include "audio/audio_file.hpp"
#include <vector>

namespace Progression {

    AudioFile::AudioFile(const std::string& filename) :
        file_(NULL),
        buffer_(-1)
    {
        alGenBuffers(1, &buffer_);
        Open(filename);
    }

    AudioFile::~AudioFile() {
        Close();
        if (buffer_ != -1)
            alDeleteBuffers(1, &buffer_);
    }

    bool AudioFile::Open(const std::string& filename) {
        if (filename == "" || file_)
            return false;

        file_ = sf_open(filename.c_str(), SFM_READ, &fileinfo_);
        return file_ != NULL;
    }

    void AudioFile::Close() {
        if (file_) {
            sf_close(file_);
            file_ = NULL;
        }
    }

    bool AudioFile::Read() {
        return ReadFileIntoBuffer(buffer_);
    }

    void AudioFile::Seek(float seconds) {
        if (!file_)
            return;
        sf_seek(file_, (sf_count_t) fileinfo_.samplerate * seconds, SEEK_SET);
    }

    bool AudioFile::ReadFileIntoBuffer(ALuint buffer) {
        if (!file_)
            return false;
        Seek(0);
        int sampleCount = fileinfo_.frames * fileinfo_.channels;
        std::vector<ALshort> fileData(sampleCount);
        sf_read_short(file_, &fileData[0], sampleCount);

        alBufferData(buffer, getALformatFromChannels(fileinfo_.channels),
                &fileData[0], sampleCount*sizeof(ALshort), fileinfo_.samplerate);

        return true;
    }

    ALenum AudioFile::getALformatFromChannels(unsigned int numChannels) {
        switch (numChannels)
        {
            case 1 : return AL_FORMAT_MONO16;
            case 2 : return AL_FORMAT_STEREO16;
            case 4 : return alGetEnumValue("AL_FORMAT_QUAD16");
            case 6 : return alGetEnumValue("AL_FORMAT_51CHN16");
            case 7 : return alGetEnumValue("AL_FORMAT_61CHN16");
            case 8 : return alGetEnumValue("AL_FORMAT_71CHN16");
            default : return 0;
        }
    }


} // namespace Progression
