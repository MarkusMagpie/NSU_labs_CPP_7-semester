#pragma once

#include <chrono>

// ГРУЗЧИК: берет задачи из общей очереди и выполняет их
class Loader {
private:
    int id_; // номер грузчика
    std::chrono::milliseconds actionTime_; // сколько длится одно действие (1-5 единиц времени)
public:
    Loader(int id, std::chrono::milliseconds actionTime);

    int id() const;
    std::chrono::milliseconds actionTime() const;
};
