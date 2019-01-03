#pragma once

#include <ostream>
#include <fstream>
#include "core/config.hpp"
#include "core/configuration.hpp"

#define LOG(s)      Logger::Write(Logger::DEBUG, s);
#define LOG_WARN(s) Logger::Write(Logger::WARN, s);
#define LOG_ERR(s)  Logger::Write(Logger::ERR, s);

class Modifier {
public:
    enum Color {
        RED = 31,
        GREEN = 32,
        YELLOW = 33,
        BLUE = 34,
        DEFAULT = 39
    };

    enum Emphasis {
        NONE = 0,
        BOLD = 1,
        UNDERLINE = 4
    };


    Modifier(Color c = DEFAULT, Emphasis e = NONE) : color(c), emphasis(e) {}

    friend std::ostream& operator<<(std::ostream& out, const Modifier& mod) {
        return out << "\033[" << mod.emphasis << ";" << mod.color << "m";
    }

    Color color;
    Emphasis emphasis;
};

class Logger {
    public:
        enum Severity {
            DEBUG,
            WARN,
            ERR
        };

        Logger() = delete;

        static void Init(const Progression::config::Config& config) {
            auto logConfig = config->get_table("logger");
            std::string filename = "";
            if (logConfig) {
                filename = PG_ROOT_DIR + logConfig->get_as<std::string>("file").value_or("");
                useColors_ = logConfig->get_as<bool>("useColors").value_or(true);
            }

            if (filename == PG_ROOT_DIR)
                return;

            out_ = std::make_unique<std::ofstream>();
            out_->open(filename);
            if (!out_->is_open()) {
                out_ = nullptr;
                LOG_ERR("Failed to open log file: \"" + filename + "\"");
            }
        }

        static void Write(Severity sev, const std::string& msg) {
            std::string severity;
            Modifier mod;
            switch (sev) {
                case DEBUG:
                    severity = "DEBUG    ";
                    mod = Modifier(Modifier::GREEN, Modifier::NONE);
                    break;
                case WARN:
                    severity = "WARNING  ";
                    mod = Modifier(Modifier::YELLOW, Modifier::NONE);
                    break;
                case ERR:
                    severity = "ERROR    ";
                    mod = Modifier(Modifier::RED, Modifier::NONE);
                    break;
            }

            if (out_) {
                (*out_) << severity << msg << std::endl;
            } else {
                if (useColors_)
                    std::cout << mod << severity << msg << Modifier() << std::endl;
                else
                    std::cout << severity << msg << std::endl;
            }
        }

        static void Free() {
            if (out_)
                out_->close();
        }

        static void useColors(bool b) { useColors_ = b; }

    private:
        static bool useColors_;
        static std::unique_ptr<std::ofstream> out_;
};

bool Logger::useColors_ = true;
std::unique_ptr<std::ofstream> Logger::out_ = {};
