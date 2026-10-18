#pragma once

#include <mutex>
#include <string>

// для потокобезопасного вывода в консоль
class Logger {
private:
    static std::mutex mutex_;
public:
    static void log(const std::string& message);
};
