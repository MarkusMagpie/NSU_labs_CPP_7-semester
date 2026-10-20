#include <string>

#include <utils/Logger.h>
#include <workers/Loader.h>

Loader::Loader(int id, std::chrono::milliseconds actionTime, 
    std::chrono::milliseconds restTime, TaskQueue& queue)
    : id_(id), 
    actionTime_(actionTime), 
    restTime_(restTime), 
    queue_(queue) {}

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

bool Loader::stopRequested() const {
    return stopToken_.stop_requested();
}

void Loader::run(std::stop_token stopToken) {
    stopToken_ = stopToken; // запоминаем, чтобы задачи могли спросить stopRequested()
    Logger::log("Грузчик #" + std::to_string(id_) + " начал смену");

    // пока никто не попросил остановиться
    while (!stopToken.stop_requested()) {
        // жду задачу не дольше restTime_
        std::shared_ptr<Task> task = queue_.pop(stopToken, restTime_);

        if (task) {
            // задаче нужны еще грузчики (в машине остались невзятые паллеты)?
            // тогда push обратно в очередь; следующий свободный грузчик присоединится
            if (task->needsMoreLoaders()) {
                queue_.push(task);
            }
            task->execute(*this);
        } else if (!stopToken.stop_requested()) {
            // nullptr не из-за остановки -> задач не было все время restTime_
            Logger::log("Грузчик #" + std::to_string(id_) + " отдохнул, так как задач в очереди не было");
        }
    }

    Logger::log("Грузчик #" + std::to_string(id_) + " закончил смену");
}
