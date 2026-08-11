#pragma once

#include <memory>
#include <string>
#include <vector>
#include <chrono>

#include <spdlog/spdlog.h>
#include <spdlog/async.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>


namespace robot
{

class Logger
{
public:

    static void init(
        const std::string& log_file = "robot.log")
    {
        if (logger_)
            return;


        // 初始化异步线程池
        spdlog::init_thread_pool(
            8192,  // queue size
            1      // logger thread
        );


        auto console_sink =
            std::make_shared<
                spdlog::sinks::stdout_color_sink_mt>();


        auto file_sink =
            std::make_shared<
                spdlog::sinks::rotating_file_sink_mt>(
                    log_file,
                    50 * 1024 * 1024, // 50MB
                    5
                );


        std::vector<spdlog::sink_ptr> sinks;

        sinks.emplace_back(console_sink);
        sinks.emplace_back(file_sink);


        logger_ =
            std::make_shared<
                spdlog::async_logger>(
                    "robot",
                    sinks.begin(),
                    sinks.end(),
                    spdlog::thread_pool(),
                    spdlog::async_overflow_policy::block
                );


        logger_->set_level(
            spdlog::level::debug
        );


        logger_->set_pattern(
            "[%Y-%m-%d %H:%M:%S.%e] "
            "[thread %t] "
            "[%^%l%$] "
            "%v"
        );


        // error以上立即刷盘
        logger_->flush_on(
            spdlog::level::err
        );


        spdlog::register_logger(
            logger_
        );


        spdlog::set_default_logger(
            logger_
        );


        // 每秒flush一次
        spdlog::flush_every(
            std::chrono::seconds(1)
        );
    }


    static void shutdown()
    {
        spdlog::shutdown();
        logger_.reset();
    }


    template<typename... Args>
    static void debug(
        fmt::format_string<Args...> fmt,
        Args&&... args)
    {
        if(logger_)
        {
            logger_->debug(
                fmt,
                std::forward<Args>(args)...
            );
        }
    }


    template<typename... Args>
    static void info(
        fmt::format_string<Args...> fmt,
        Args&&... args)
    {
        if(logger_)
        {
            logger_->info(
                fmt,
                std::forward<Args>(args)...
            );
        }
    }


    template<typename... Args>
    static void warn(
        fmt::format_string<Args...> fmt,
        Args&&... args)
    {
        if(logger_)
        {
            logger_->warn(
                fmt,
                std::forward<Args>(args)...
            );
        }
    }


    template<typename... Args>
    static void error(
        fmt::format_string<Args...> fmt,
        Args&&... args)
    {
        if(logger_)
        {
            logger_->error(
                fmt,
                std::forward<Args>(args)...
            );
        }
    }


    template<typename... Args>
    static void critical(
        fmt::format_string<Args...> fmt,
        Args&&... args)
    {
        if(logger_)
        {
            logger_->critical(
                fmt,
                std::forward<Args>(args)...
            );
        }
    }


private:

    inline static std::shared_ptr<spdlog::logger> logger_ = nullptr;

};

} // namespace robot



// =========================
// 用户接口
// =========================

#define LOG_DEBUG(...) \
    robot::Logger::debug(__VA_ARGS__)


#define LOG_INFO(...) \
    robot::Logger::info(__VA_ARGS__)


#define LOG_WARN(...) \
    robot::Logger::warn(__VA_ARGS__)


#define LOG_ERROR(...) \
    robot::Logger::error(__VA_ARGS__)


#define LOG_CRITICAL(...) \
    robot::Logger::critical(__VA_ARGS__)