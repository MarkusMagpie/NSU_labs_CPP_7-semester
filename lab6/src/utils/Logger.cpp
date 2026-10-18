#include <iostream>

#include <utils/Logger.h>

// статик объявлен в .h
std::mutex Logger::mutex_;

void Logger::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_); // mutex.lock()
    std::cout << message << '\n';
} // деструктор std::lock_guard -> mutex.unlock()
