#include "Logger.h"
#include <spdlog/sinks/rotating_file_sink.h>
#include <yaml-cpp/yaml.h>

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

Logger::Logger() {
    uint32_t logLevel = 2;
    std::string logPath = "./logs";
    uint32_t rotateSize = 100;
    uint32_t maxFiles = 100;
    try {
        YAML::Node config = YAML::LoadFile("./config/config.yml");
        if (!config.IsNull()) {
            if (config["Logger"]) {
                auto loggerNode = config["Logger"];
                if (loggerNode["level"]) {
                    logLevel = loggerNode["level"].as<uint32_t>();
                }
                if (loggerNode["rotateSize"]) {
                    rotateSize = loggerNode["rotateSize"].as<uint32_t>();
                }
                if (loggerNode["maxFiles"]) {
                    maxFiles = loggerNode["maxFiles"].as<uint32_t>();
                }
                if (loggerNode["path"]) {
                    logPath = loggerNode["path"].as<std::string>();
                }
            }
        }
    }
    catch (const std::exception& e) {
        LogErr("{}",e.what());
    }
    mpThreadPool = std::make_shared<spdlog::details::thread_pool>(2,2);
    mpFileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logPath + "/log.log",1024 * 1024 * rotateSize,maxFiles);
    mpFileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
    mpConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    mpConsoleSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
    std::vector sinks {mpFileSink, mpConsoleSink};
    mpLogger = std::make_shared<spdlog::async_logger>("Logger", sinks.begin(), sinks.end(),mpThreadPool,spdlog::async_overflow_policy::block);
    if (logLevel > 0 && logLevel < spdlog::level::n_levels) {
        mpLogger->set_level(static_cast<spdlog::level::level_enum>(logLevel));
    }
}

Logger::~Logger() {
    mpLogger->flush();
}

void Logger::setLevel(spdlog::level::level_enum level) const {
    mpLogger->set_level(level);
}

void Logger::log(const std::string &msg, const std::string& file, int line, spdlog::level::level_enum level) const {
    const std::string content = fmt::format("[{}:{}] : {}",file,line, msg);
    mpLogger->log(level,content);
    mpLogger->flush();
}
