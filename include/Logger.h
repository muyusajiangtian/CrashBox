#pragma once
// 日志模块 - 简单日志输出

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace CrashBox {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR_LEVEL
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void init(const std::string& logFile = "") {
        if (!logFile.empty()) {
            fileStream.open(logFile, std::ios::out | std::ios::trunc);
            if (fileStream.is_open()) {
                useFile = true;
            }
        }
        log(LogLevel::INFO, "CrashBox 日志系统初始化完成");
    }

    void setLevel(LogLevel level) { minLevel = level; }

    void log(LogLevel level, const std::string& msg) {
        if (level < minLevel) return;

        std::string prefix;
        switch (level) {
            case LogLevel::DEBUG: prefix = "[调试]"; break;
            case LogLevel::INFO: prefix = "[信息]"; break;
            case LogLevel::WARNING: prefix = "[警告]"; break;
            case LogLevel::ERROR_LEVEL: prefix = "[错误]"; break;
        }

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time), "%H:%M:%S") << " " << prefix << " " << msg;

        std::string output = oss.str();
        std::cout << output << std::endl;
        if (useFile && fileStream.is_open()) {
            fileStream << output << std::endl;
        }
    }

    void debug(const std::string& msg) { log(LogLevel::DEBUG, msg); }
    void info(const std::string& msg) { log(LogLevel::INFO, msg); }
    void warn(const std::string& msg) { log(LogLevel::WARNING, msg); }
    void error(const std::string& msg) { log(LogLevel::ERROR_LEVEL, msg); }

    ~Logger() {
        if (fileStream.is_open()) fileStream.close();
    }

private:
    Logger() = default;
    LogLevel minLevel = LogLevel::INFO;
    std::ofstream fileStream;
    bool useFile = false;
};

// 便捷宏
#define LOG_DEBUG(msg) CrashBox::Logger::instance().debug(msg)
#define LOG_INFO(msg) CrashBox::Logger::instance().info(msg)
#define LOG_WARN(msg) CrashBox::Logger::instance().warn(msg)
#define LOG_ERROR(msg) CrashBox::Logger::instance().error(msg)

} // namespace CrashBox
