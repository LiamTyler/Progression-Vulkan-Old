#pragma once

#include <string>
#include <sys/stat.h>
#include "utils/logger.hpp"

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
            struct stat s;
            valid = stat(filename.c_str(), &s) == 0;
            timestamp = s.st_mtime;
        }

        bool update() {
            struct stat s;
            valid = stat(filename.c_str(), &s) == 0;
            if (!valid)
                return false;
            time_t newTimestamp = s.st_mtime;
            bool ret = timestamp != newTimestamp;
            timestamp = newTimestamp;
            return ret;
        }

        bool outOfDate(const TimeStampedFile& ts) const {
            return (filename != ts.filename) || (!valid && ts.valid) || (valid && ts.valid && timestamp < ts.timestamp);
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

    class MetaData {};

    class Resource {
    public:
        Resource() : name("") {}
        Resource(const std::string& n) : name(n) {}
        virtual ~Resource() = default;

        virtual bool load(MetaData* metaData = nullptr) = 0;
        virtual bool loadFromResourceFile(std::istream& in) = 0;
        virtual void move(Resource* resource) = 0;
        virtual Resource* needsReloading() { return nullptr; }

        std::string name;
    };

} // namespace Progression
