#pragma once

#include <string>

// базовый, абстрактный для задач грузчика (разгрузка, инвентаризация, комплектация)
class Task {
public:
    virtual ~Task() = default;

    virtual void execute() = 0;
    virtual std::string name() const = 0;
};
