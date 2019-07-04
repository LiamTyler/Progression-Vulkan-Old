#pragma once

#include <string>
#include <sys/stat.h>
#include "utils/logger.hpp"
#include <functional>

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

    enum ResUpdateStatus {
        RES_PARSE_ERROR,
        RES_RELOAD_SUCCESS,
        RES_RELOAD_FAILED,
        RES_UPDATED,
        RES_UP_TO_DATE
    };

    class Resource {
    public:
        Resource() : name("") {}
        Resource(const std::string& n) : name(n) {}
        virtual ~Resource() = default;

        virtual bool load(MetaData* metaData = nullptr) = 0;
        virtual ResUpdateStatus loadFromResourceFile(std::istream& in, std::function<void()>& updateFunc) = 0;
        virtual void move(Resource* resource) = 0;
        virtual std::shared_ptr<Resource> needsReloading() { return nullptr; }

        std::string name;
    };

} // namespace Progression
