#pragma once

#include <AL/al.h>
#include <AL/alc.h>
#include <sndfile.h>
#include <string>

namespace Progression {

    class AudioFile {
        public:
            AudioFile(const std::string& filepath = "");
            ~AudioFile();
            AudioFile(const AudioFile&) = delete;
            AudioFile& operator=(const AudioFile&) = delete;

            bool Open(const std::string& filepath);
            void Close();
            bool Read();

            void Seek(float seconds);
            bool ReadFileIntoBuffer(ALuint buffer);

            bool isOpen() const { return file_ != NULL; }
            const SF_INFO& getFileInfo() const { return fileinfo_; }
            ALuint getBuffer() const { return buffer_; }
            
        private:
            static ALenum getALformatFromChannels(unsigned int numChannels);

            ALuint buffer_;
            SNDFILE* file_;
            SF_INFO fileinfo_;
    };

} // namespace Progression
