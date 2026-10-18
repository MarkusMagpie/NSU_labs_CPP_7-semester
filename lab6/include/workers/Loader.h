#pragma once

#include <chrono>
#include <stop_token>
#include <thread>

// ГРУЗЧИК: берет задачи из общей очереди и выполняет их
class Loader {
private:
    int id_; // номер грузчика
    std::chrono::milliseconds actionTime_; // сколько длится одно действие (1-5 единиц времени)
    std::jthread thread_;
public:
    Loader(int id, std::chrono::milliseconds actionTime);

    // грузчика нельзя копировать; так бы копировался поток
    Loader(const Loader&) = delete;
    Loader& operator=(const Loader&) = delete;

    int id() const;
    std::chrono::milliseconds actionTime() const;

    // запустить поток грузчика; поток выполняет run()
    void start_thread();
    // попросить поток остановиться (он заметит это на следующей проверке stop_requested)
    void request_stop();
    // дождаться завершения потока
    void join();

    // рабочий цикл грузчика; выполняется в его собственном потоке
    void run(std::stop_token stopToken);
};
