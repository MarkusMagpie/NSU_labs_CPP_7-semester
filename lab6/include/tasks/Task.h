#pragma once

#include <string>

class Loader;

// чем закончилось выполнение задачи грузчиком
enum class TaskResult {
    Done, // грузчик закончил с задачей (выполнил или отказался) -> в очередь ее не возвращать
    Requeue // задачу не удалось продолжить сейчас - грузчик возвращает ее в очередь с состоянием
};

// базовый, абстрактный для задач грузчика (разгрузка, инвентаризация, комплектация)
class Task {
public:
    virtual ~Task() = default;

    virtual TaskResult execute(Loader& loader) = 0;
    virtual std::string name() const = 0;

    // нужны ли задаче еще рабочие:
    // да -> взявший возвращает задачу в очередь, чтобы к работе подключились другие
    virtual bool needsMoreLoaders() { 
        return false; 
    }
};
