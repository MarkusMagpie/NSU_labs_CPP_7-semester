#pragma once

// МЕНЕДЖЕР: ставит задачи для работы склада (разгрузка, инвентаризация, комплектация)
// и оповещает о приходе машин для отправки заказов
class Manager {
private:
    int id_;
public:
    Manager(int id);

    int id() const;
};
