#include <iostream>

#include <common/Logger.hpp>

std::mutex Logger::mutex_;

void Logger::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << message << '\n';
}
