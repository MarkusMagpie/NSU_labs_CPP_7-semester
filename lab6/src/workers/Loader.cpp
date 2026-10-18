#include <string>

#include <utils/Logger.h>
#include <workers/Loader.h>

Loader::Loader(int id, std::chrono::milliseconds actionTime) : id_(id), actionTime_(actionTime) {}

int Loader::id() const {
    return id_;
}

std::chrono::milliseconds Loader::actionTime() const {
    return actionTime_;
}

void Loader::start_thread() {
    // jthread создаст stop_token и передаст его в лямбду аргументом
    // [this] - захват указателя this текущего объекта -> run()
    thread_ = std::jthread([this](std::stop_token stopToken) {
        run(stopToken);
    });
}

void Loader::request_stop() {
    // поднять флаг остановки для std::stop_token stopToken (stopToken.stop_requested())
    // https://en.cppreference.com/cpp/thread/jthread/request_stop
    thread_.request_stop();
}

void Loader::join() {
    // joinable() == false если поток не запускали или уже дождались
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Loader::run(std::stop_token stopToken) {
    Logger::log("Грузчик #" + std::to_string(id_) + " начал смену");

    // пока никто не попросил остановиться
    while (!stopToken.stop_requested()) {
        Logger::log("Грузчик #" + std::to_string(id_) + " отдыхает");
        std::this_thread::sleep_for(actionTime_);
    }

    Logger::log("Грузчик #" + std::to_string(id_) + " закончил смену");
}
