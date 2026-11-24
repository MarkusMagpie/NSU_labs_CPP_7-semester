#pragma once

#include <cstddef>
#include <memory>

#include <ComplexTypes.hpp>

// таблица счетов: массив фиксированной вместимости (capacity) и число занятых элементов
class AccountTable {
private:
    std::unique_ptr<Account[]> accounts_;
    std::size_t size_ = 0; // сколько записей в таблице заполнено
public:
    static constexpr std::size_t capacity = 1e5;

    AccountTable();

    bool add(const Account& account);
    Account* findById(unsigned long long id);
    const Account* findById(unsigned long long id) const;
    std::size_t size() const;
};
