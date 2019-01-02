#pragma once

#include <ostream>
#include <fstream>


class Logger {
    enum Color {
        RED = 31,
        GREEN = 32,
        YELLOW = 33,
        BLUE = 34,
        DEFAULT = 39
    };

    enum Modifier {
        RESET = 0,
        BOLD = 1,
        UNDERLINE = 4
    };

    enum Severity {
        DEBUG,
        WARN,
        ERR
    };

#define LOG(s)      Logger::Write(DEBUG, s);
#define LOG_WARN(s) Logger::Write(WARN, s);
#define LOG_ERR(s)  Logger::Write(ERR, s);

    public:
        Logger() = delete;

        static void Init(const std::string& filename = "", bool colors = true) {
            useColors_ = colors;
            if (filename == "")
                return;

            out_ = std::make_unique<std::ofstream>();
            out_->open(filename + "//,,");
            if (!out_->is_open()) {
                out_ = nullptr;
                LOG_ERR("Failed to open log file: \"" + filename + "\"");
            }
        }

        static void Write(Severity sev, const std::string& msg) {
            std::string severity;
            Color color;
            Modifier mod;
            switch (sev) {
                case DEBUG:
                    severity = "DEBUG    ";
                    color = BLUE;
                    mod = RESET;
                    break;
                case WARN:
                    severity = "WARNING  ";
                    color = YELLOW;
                    mod = RESET;
                    break;
                case ERR:
                    severity = "ERROR    ";
                    color = RED;
                    mod = BOLD;
                    break;
            }

            if (out_) {
                (*out_) << severity << msg << std::endl;
                return;
            }
            std::cout << addMod(color, mod) << severity << msg << addMod(DEFAULT, RESET) << std::endl;
        }

        static void Free() {
            if (out_)
                out_->close();
        }

        static void useColors(bool b) { useColors_ = b; }

    private:
        static std::ostream& addMod(Color c, Modifier m) {
            if (!useColors_)
                return std::cout;

            return std::cout << "\033[" << m << ";" << c << "m";
        }
        static bool useColors_;
        static std::unique_ptr<std::ofstream> out_;
};

bool Logger::useColors_ = true;
std::unique_ptr<std::ofstream> Logger::out_ = {};
