#include "utils/logger.hpp"

bool Logger::useColors_ = true;
std::unique_ptr<std::ofstream> Logger::out_ = {};