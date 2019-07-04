#include "utils/logger.hpp"

bool Logger::useColors_ = true;
std::unique_ptr<std::ofstream> Logger::out_ = {};
std::mutex Logger::lock_;