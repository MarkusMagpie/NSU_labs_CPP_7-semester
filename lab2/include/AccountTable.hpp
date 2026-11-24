#pragma once

#include <cstddef>
#include <ComplexTypes.hpp>
#include <Table.hpp>

// таблица счетов: массив фиксированной вместимости (capacity) и число занятых элементов
class AccountTable {
private:
    Table<Account> accounts_;
public:
    static constexpr std::size_t capacity = Table<Account>::capacity;

    bool add(const Account& account);
    Account* findById(unsigned long long id);
    const Account* findById(unsigned long long id) const;
    std::size_t size() const;
};
