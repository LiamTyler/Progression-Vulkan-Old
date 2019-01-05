#pragma once

#include <iostream>
#include <fstream>
#include "core/config.hpp"
#include "core/configuration.hpp"

#define LOG(...)      Logger::Write(Logger::DEBUG, __VA_ARGS__);
#define LOG_WARN(...) Logger::Write(Logger::WARN, __VA_ARGS__);
#define LOG_ERR(...)  Logger::Write(Logger::ERR, __VA_ARGS__);

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

        template <typename T>
        static void printArgs(std::ostream& out, T t)
        {
            out << t << std::endl;
        }

        template <typename T, typename U, typename... Args>
        static void printArgs(std::ostream& out, T t, U u, Args... args)
        {
            out << t << " ";
            printArgs(out, u, args...);
        }

        template <typename...Args>
        static void Write(Severity sev, Args... args) {
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
                printArgs(*out_, severity, args...);
            }
            else {
                if (useColors_) {
                    std::cout << mod << severity;
                    printArgs(std::cout, args...);
                    std::cout << Modifier();
                } else {
                    printArgs(std::cout, severity, args...);
                }
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
