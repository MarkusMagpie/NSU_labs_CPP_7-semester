#pragma once

#include <mutex>
#include <string>

// потокобезопасный вывод в консоль
class Logger {
public:
    static void log(const std::string& message);

private:
    static std::mutex mutex_;
};
