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

std::stop_token Loader::stopToken() const {
    return stopToken_;
}

void Loader::run(std::stop_token stopToken) {
    stopToken_ = stopToken; // чтобы задачи могли спросить stopRequested()
    Logger::log("Грузчик #" + std::to_string(id_) + " начал смену");

    // пока никто не попросил остановиться
    while (!stopToken.stop_requested()) {
        // 1) вынул задачу; в очереди ее больше нет
        std::shared_ptr<Task> task = queue_.pop(stopToken, restTime_);

        if (task) {
            // 2) задаче еще нужны грузчики?
            if (task->needsMoreLoaders()) {
                // 2) да -> сразу кладу ту же задачу обратно в очередь
                queue_.push(task);
            }
            // 3) сам вне зависимости от 2) начинаю делать задачу
            // 4) задача не смогла продолжиться (нет стола || нет машины || хз уже) 
            // -> возвращаю ее в очередь
            if (task->execute(*this) == TaskResult::Requeue) {
                queue_.push(task);
                Logger::log("Грузчик #" + std::to_string(id_) + " вернул задачу \"" + task->name() + "\" в очередь");
            }
        } else if (!stopToken.stop_requested()) {
            Logger::log("Грузчик #" + std::to_string(id_) + " отдохнул, так как задач в очереди не было");
        }
    }

    Logger::log("Грузчик #" + std::to_string(id_) + " закончил смену");
}
