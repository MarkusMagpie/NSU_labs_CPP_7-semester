#pragma once

#include <cstddef>
#include <ComplexTypes.hpp>
#include <Table.hpp>

// таблица вкладов: массив фиксированной вместимости и число занятых элементов
class DepositTable {
private:
    Table<Deposit> deposits_;

public:
    static constexpr std::size_t capacity = Table<Deposit>::capacity;

    bool add(const Deposit& deposit);
    Deposit* findById(unsigned long long id);
    const Deposit* findById(unsigned long long id) const;
    std::size_t size() const;
};
