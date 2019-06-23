#pragma once

#include <string>
#include <sys/stat.h>

namespace Progression {

    class TimeStampedFile {
    public:
        TimeStampedFile() :
            valid(false),
            filename("")
        {
        }

        TimeStampedFile(const std::string& fname) :
            valid(false),
            filename(fname)
        {
            if (filename != "") {
                struct stat s;
                valid = stat(fname.c_str(), &s) == 0;
                timestamp = s.st_mtime;
            }
        }
        
        // assumes that the filenames are the same
        bool outOfDate(const TimeStampedFile& ts) const {
            return (!valid && ts.valid) || (valid && ts.valid && timestamp < ts.timestamp);
        }

        bool operator==(const TimeStampedFile& f) const {
            return filename == f.filename && valid == f.valid && timestamp == f.timestamp;
        }

        bool operator!=(const TimeStampedFile& f) const {
            return !(*this == f);
        }

        time_t timestamp;
        bool valid;
        std::string filename;
    };

    class Resource {
    public:
        Resource() : name("") {}
        Resource(const std::string& n) : name(n) {}

        std::string name;
    };

} // namespace Progression
