#include "Logger.h"
#include <QThread>
#include <spdlog/sinks/rotating_file_sink.h>
#include <yaml-cpp/yaml.h>
#include <spdlog/sinks/stdout_color_sinks.h>

Logger* Logger::mpInstance = nullptr;
std::mutex Logger::mMutex;
Logger* Logger::getInstance() {
    if (mpInstance == nullptr) {
        std::scoped_lock lock(mMutex);
        if (nullptr == mpInstance) {
            mpInstance = new Logger();
        }
    }
    return mpInstance;
}

Logger::Logger() = default;

Logger::~Logger() {
    if (nullptr != mpLogger) {
        mpLogger->flush();
    }
}

void Logger::loadConfig(const std::string &configFile) {
    uint32_t logLevel = 2;
    std::string logPath = "./logs";
    uint32_t rotateSize = 100;
    uint32_t maxFiles = 100;
    try {
        YAML::Node config = YAML::LoadFile(configFile);
        if (!config.IsNull()) {
            if (config["logger"]) {
                auto loggerNode = config["logger"];
                if (loggerNode["level"]) {
                    logLevel = loggerNode["level"].as<uint32_t>();
                }
                if (loggerNode["rotate_size"]) {
                    rotateSize = loggerNode["rotate_size"].as<uint32_t>();
                }
                if (loggerNode["max_files"]) {
                    maxFiles = loggerNode["max_files"].as<uint32_t>();
                }
                if (loggerNode["path"]) {
                    logPath = loggerNode["path"].as<std::string>();
                }
            }
        }
    }
    catch (const std::exception& e) {
        fmt::println("Logger::loadConfig Error : {}", e.what());
    }
    mpFileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logPath + "/log.log",1024 * 1024 * rotateSize,maxFiles);
    mpFileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
    mpConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    mpConsoleSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
    std::vector sinks {mpFileSink, mpConsoleSink};
    mpLogger = std::make_shared<spdlog::logger>("Logger", sinks.begin(), sinks.end());
    if (logLevel > 0 && logLevel < spdlog::level::n_levels) {
        mpLogger->set_level(static_cast<spdlog::level::level_enum>(logLevel));
    }
}


void Logger::setLevel(spdlog::level::level_enum level) const {
    if (nullptr != mpLogger) {
        mpLogger->set_level(level);
    }
}

void Logger::log(const std::string &msg, const std::string& file, int line, spdlog::level::level_enum level) const {
    if (nullptr != mpLogger) {
        const std::string content = fmt::format("[{}:{}] : {}",file,line, msg);
        mpLogger->log(level,content);
        mpLogger->flush();
    }
}
