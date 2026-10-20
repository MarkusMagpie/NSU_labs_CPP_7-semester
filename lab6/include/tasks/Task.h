#pragma once

#include <string>

class Loader;

// базовый, абстрактный для задач грузчика (разгрузка, инвентаризация, комплектация)
class Task {
public:
    virtual ~Task() = default;

    // loader - грузчик, который выполняет задачу (одну задачу могут выполнять несколько грузчиков)
    virtual void execute(Loader& loader) = 0;
    virtual std::string name() const = 0;

    // нужны ли задаче еще рабочие:
    // да -> взявший возвращает задачу в очередь, чтобы к работе подключились другие
    virtual bool needsMoreLoaders() { 
        return false; 
    }
};
