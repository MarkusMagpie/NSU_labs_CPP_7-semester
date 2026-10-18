#include <string>

#include <utils/Logger.h>
#include <workers/Manager.h>

Manager::Manager(int id, std::chrono::milliseconds period) : id_(id), period_(period) {}

int Manager::id() const {
    return id_;
}

void Manager::start_thread() {
    // jthread создаст stop_token и передаст его в лямбду аргументом
    thread_ = std::jthread([this](std::stop_token stopToken) {
        run(stopToken);
    });
}

void Manager::request_stop() {
    thread_.request_stop();
}

void Manager::join() {
    // joinable() == false, если поток не запускали или уже дождались
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Manager::run(std::stop_token stopToken) {
    Logger::log("Менеджер #" + std::to_string(id_) + " начал смену");

    while (!stopToken.stop_requested()) {
        Logger::log("Менеджер #" + std::to_string(id_) + " работает");
        std::this_thread::sleep_for(period_);
    }

    Logger::log("Менеджер #" + std::to_string(id_) + " закончил смену");
}
