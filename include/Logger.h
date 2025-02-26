//
// Created by cumtzt on 25-1-21.
//

#ifndef OPCCLIENT_LOGGER_H
#define OPCCLIENT_LOGGER_H

#include <spdlog/async_logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <QString>
#include <spdlog/async.h>
class Logger{

public:

    static Logger* getInstance();

    Logger(Logger const&) = delete;

    Logger(Logger&&) = delete;

    Logger& operator=(Logger const&) = delete;

    ~Logger();

    void setLevel(spdlog::level::level_enum level) const;

    void log(const std::string& msg, const std::string& file, int line, spdlog::level::level_enum level) const;

private:

    Logger();

    static Logger* mpInstance;

    static std::mutex mMutex;

    spdlog::sink_ptr mpFileSink = nullptr;

    spdlog::sink_ptr mpConsoleSink = nullptr;

    spdlog::details::async_logger_ptr mpLogger = nullptr;

    std::shared_ptr<spdlog::details::thread_pool> mpThreadPool = nullptr;

};

#define LoggerIns Logger::getInstance()
#define LogTrace(...) LoggerIns->log(fmt::format(__VA_ARGS__), __FILE_NAME__, __LINE__,spdlog::level::trace)
#define LogDebug(...) LoggerIns->log(fmt::format(__VA_ARGS__), __FILE_NAME__, __LINE__, spdlog::level::debug)
#define LogInfo(...) LoggerIns->log(fmt::format(__VA_ARGS__), __FILE_NAME__, __LINE__, spdlog::level::info)
#define LogWarn(...) LoggerIns->log(fmt::format(__VA_ARGS__), __FILE_NAME__, __LINE__, spdlog::level::warn)
#define LogErr(...) LoggerIns->log(fmt::format(__VA_ARGS__), __FILE_NAME__, __LINE__, spdlog::level::err)
#define LogCritical(...) LoggerIns->log(fmt::format(__VA_ARGS__), __FILE_NAME__, __LINE__, spdlog::level::critical)

#endif //OPCCLIENT_LOGGER_H
