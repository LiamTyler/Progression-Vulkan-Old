#pragma once

#include <string>
#include <sys/stat.h>
#include "utils/logger.hpp"
#include <functional>

namespace Progression {

    class TimeStampedFile {
    public:
        TimeStampedFile();
        TimeStampedFile(const std::string& fname);
        bool update();
        bool outOfDate(const TimeStampedFile& ts) const;
        bool operator==(const TimeStampedFile& f) const;
        bool operator!=(const TimeStampedFile& f) const;
        void save(std::ofstream& out) const;
        void load(std::ifstream& in);
        friend std::ostream& operator<<(std::ostream& out, const TimeStampedFile& ts);

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
        virtual bool readMetaData(std::istream& in) = 0;
        virtual ResUpdateStatus loadFromResourceFile(std::istream& in, std::function<void()>& updateFunc) = 0;
        virtual void move(Resource* resource) = 0;
        virtual std::shared_ptr<Resource> needsReloading() { return nullptr; }
        virtual bool saveToFastFile(std::ofstream& outFile) const { (void)outFile; return false; }
        virtual bool loadFromFastFile(std::ifstream& in) { (void)(in); return false; }

        std::string name;
    };

} // namespace Progression
