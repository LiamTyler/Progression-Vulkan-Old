#include "resource/resource.hpp"
#include "utils/serialize.hpp"

namespace Progression {

    TimeStampedFile::TimeStampedFile() :
        valid(false),
        filename("")
    {
    }

    TimeStampedFile::TimeStampedFile(const std::string& fname) :
        valid(false),
        filename(fname)
    {
        struct stat s;
        valid = stat(filename.c_str(), &s) == 0;
        timestamp = s.st_mtime;
    }

    bool TimeStampedFile::update() {
        struct stat s;
        valid = stat(filename.c_str(), &s) == 0;
        if (!valid)
            return false;
        time_t newTimestamp = s.st_mtime;
        bool ret = timestamp != newTimestamp;
        timestamp = newTimestamp;
        return ret;
    }

    bool TimeStampedFile::outOfDate(const TimeStampedFile& ts) const {
        return (filename != ts.filename) || (!valid && ts.valid) || (valid && ts.valid && timestamp < ts.timestamp);
    }

    bool TimeStampedFile::operator==(const TimeStampedFile& f) const {
        return filename == f.filename && valid == f.valid && timestamp == f.timestamp;
    }

    bool TimeStampedFile::operator!=(const TimeStampedFile& f) const {
        return !(*this == f);
    }

    void TimeStampedFile::save(std::ofstream& out) const {
        serialize::write(out, timestamp);
        serialize::write(out, filename);
    }

    void TimeStampedFile::load(std::ifstream& in) {
        time_t ts;
        serialize::read(in, ts);
        serialize::read(in, filename);
        *this = TimeStampedFile(filename);
        timestamp = ts;
    }

    std::ostream& operator<<(std::ostream& out, const TimeStampedFile& ts) {
        return out << "File '" << ts.filename << "', timestamp: " << ts.timestamp << ", valid: " << ts.valid; 
    }

} // namespace Progression